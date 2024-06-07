/*
   Copyright (c) 2021 Fraunhofer AISEC. See the COPYRIGHT
   file at the top-level directory of this distribution.

   Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
   http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
   <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
   option. This file may not be copied, modified, or distributed
   except according to those terms.
*/

#include <stdbool.h>
#include <stdint.h>

#include "edhoc.h"

#include "edhoc/cert.h"

#include "common/memcpy_s.h"
#include "common/oscore_edhoc_error.h"
#include "common/crypto_wrapper.h"

#include "cbor/edhoc_decode_cert.h"

#ifdef MBEDTLS
#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#include <psa/crypto.h>
#include <mbedtls/asn1.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>

struct deser_sign_ctx_s {
	uint8_t *seek;
	uint8_t *end;
	int unit_size;
};

static void deser_sign_ctx_init(struct deser_sign_ctx_s *ctx, uint8_t *seek,
				uint8_t *end, int unit_size)
{
	ctx->seek = seek;
	ctx->end = end;
	ctx->unit_size = unit_size;
}

static int deser_sign_cb(void *void_ctx, int tag, unsigned char *start,
			 size_t len)
{
	if (tag == MBEDTLS_ASN1_INTEGER) {
		struct deser_sign_ctx_s *ctx = void_ctx;
		uint8_t *unit_end = ctx->seek + ctx->unit_size;
		if (unit_end <= ctx->end) {
			memcpy(ctx->seek, start + len - ctx->unit_size,
			       (uint32_t)ctx->unit_size);
			ctx->seek = unit_end;
		}
	}
	return 0;
}

static int find_pk_cb(void *void_ppk, int tag, unsigned char *start, size_t len)
{
	(void)len;

	if (tag == MBEDTLS_ASN1_BIT_STRING) {
		uint8_t **pk = void_ppk;
		*pk = start;
	}
	return 0;
}

#define PSA_KEY_ALL_USAGES                                                     \
	(PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY | PSA_KEY_USAGE_ENCRYPT |   \
	 PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_SIGN_MESSAGE |                  \
	 PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_SIGN_HASH |              \
	 PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_DERIVE)

#else /* MBEDTLS */

#define ISSUER_CN_OID "\x55\x04\x03"

#define EXPECTO_TAG(tag, cursor, len)									\
	if (*cursor != tag) {									\
		rv = wrong_parameter;								\
		PRINTF(RED											\
			"Runtime error: expected %s tag at %s:%d\n\n"	\
			RESET, #tag, __FILE__, __LINE__);				\
		break;												\
	} else {												\
		cursor++;											\
		mbedtls_asn1_get_len(&cursor, end, &len);			\
		if (0 == *cursor) {cursor++; len--;}				\
	}

enum tag_map_enum {
	ASN1_INTEGER = 0x02,
	ASN1_BIT_STRING = 0x03,
	ASN1_SEQUENCE = 0x30
};


/* Extracted from mbedtls library; asn1_parse.c */
/* License for file: Apache-2.0 */
/* ASN.1 DER encoding is described in ITU-T X.690 standard. */
/* First bit of length byte contains information, if length
   value shall be concatenated with following byte length. */
static int
mbedtls_asn1_get_len(const unsigned char **p, const unsigned char *end,
			 size_t *len) 
{
	if ((end - *p) < 1)
		return (buffer_to_small);

	if ((**p & 0x80) == 0)
		*len = *(*p)++;
	else {
		switch (**p & 0x7F) {
		case 1:
			if ((end - *p) < 2)
				return (buffer_to_small);

			*len = (*p)[1];
			(*p) += 2;
			break;

		case 2:
			if ((end - *p) < 3)
				return (buffer_to_small);

			*len = ((size_t)(*p)[1] << 8) | (*p)[2];
			(*p) += 3;
			break;

		case 3:
			if ((end - *p) < 4)
				return (buffer_to_small);

			*len = ((size_t)(*p)[1] << 16) |
			       ((size_t)(*p)[2] << 8) | (*p)[3];
			(*p) += 4;
			break;

		case 4:
			if ((end - *p) < 5)
				return (buffer_to_small);

			*len = ((size_t)(*p)[1] << 24) |
			       ((size_t)(*p)[2] << 16) |
			       ((size_t)(*p)[3] << 8) | (*p)[4];
			(*p) += 5;
			break;

		default:
			return (buffer_to_small);
		}
	}

	if (*len > (size_t)(end - *p))
		return (buffer_to_small);

	return (0);
}

#endif /* MBEDTLS */

/**
 * @brief retrives the public key of the CA from CRED_ARRAY.
 * 
 * 
 * @param cred_array contains the public key of the root CA
 * @param cred_num the number of elements in cred_array
 * @param issuer the issuer name, i.e. the name of the CA
 * @param root_pk the root public key
 * @param root_pk_len the lenhgt of the root public key
 * @return enum err 
 */
static enum err ca_pk_get(const struct other_party_cred *cred_array,
			  uint16_t cred_num, const uint8_t *issuer,
			  uint8_t **root_pk, uint32_t *root_pk_len)
{
	/* when single credential without certificate is stored, return stored ca_pk if available */
	if(1 == cred_num
#ifdef MBEDTLS
	/* In case no MBEDTLS is enabled, issuer identification is not extracted from certificate */
	&& ( 0 == cred_array[0].ca.len || NULL == cred_array[0].ca.ptr)
#endif
		)
	{
		if(NULL == cred_array[0].ca_pk.ptr || 0 == cred_array[0].ca_pk.len)
		{
			return no_such_ca;
		}

		*root_pk = cred_array[0].ca_pk.ptr;
		*root_pk_len = cred_array[0].ca_pk.len;
		return ok;
	}

#ifdef MBEDTLS
	/* Accept only certificate based search if multiple credentials available*/
	for (uint16_t i = 0; i < cred_num; i++) {
		if(NULL == cred_array[i].ca.ptr || 0 == cred_array[i].ca.len)
		{
			continue;
		}

		PRINT_ARRAY("cred_array[i].ca.ptr", cred_array[i].ca.ptr,
			    cred_array[i].ca.len);
		PRINT_ARRAY("issuer", issuer, cred_array[i].ca.len);

		mbedtls_x509_crt m_cert;
		mbedtls_x509_crt_init(&m_cert);

		/* parse the certificate */
		TRY_EXPECT(mbedtls_x509_crt_parse_der_nocopy(&m_cert, cred_array[i].ca.ptr, cred_array[i].ca.len), 0);

		const mbedtls_x509_name *p = &m_cert.subject;
		const mbedtls_asn1_buf *subject_id = NULL;
		while (p) {

			if (0 == MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &p->oid)) {
				subject_id = &p->val;
			}
			p = p->next;
		};

		if (0 == memcmp(subject_id->p, issuer,
				subject_id->len)) {
			*root_pk = cred_array[i].ca_pk.ptr;
			*root_pk_len = cred_array[i].ca_pk.len;
			PRINT_ARRAY("Root PK of the CA", *root_pk,
				    *root_pk_len);
			mbedtls_x509_crt_free(&m_cert);
			return ok;
		}
		else{
			mbedtls_x509_crt_free(&m_cert);
		}
	}
#endif /* MBEDTLS */

	return no_such_ca;
}

enum err cert_c509_verify(const uint8_t *cert, uint32_t cert_len,
			  const struct other_party_cred *cred_array,
			  uint16_t cred_num, uint8_t *pk, uint32_t *pk_len,
			  bool *verified)
{
	size_t decode_len = 0;
	struct cert c;

	TRY_EXPECT(cbor_decode_cert(cert, cert_len, &c, &decode_len), true);

	PRINT_MSG("CBOR certificate parsed.\n");
	PRINTF("Certificate type: %d\n", c._cert_type);
	// PRINT_ARRAY("Serial number", c._cert_serial_number.value,
	// 	    c._cert_serial_number.len);
	PRINT_ARRAY("issuer", c._cert_issuer.value,
		    (uint32_t)c._cert_issuer.len);
	PRINTF("validity_not_before: %d\n", c._cert_validity_not_before);
	PRINTF("validity_not_after: %d\n", c._cert_validity_not_after);
	PRINT_ARRAY("subject", c._cert_subject.value,
		    (uint32_t)c._cert_subject.len);
	PRINT_ARRAY("PK", c._cert_pk.value, (uint32_t)c._cert_pk.len);
	PRINTF("extensions: %d\n", c._cert_extensions);
	PRINTF("issuer_signature_algorithm: %d\n",
	       c._cert_issuer_signature_algorithm);
	PRINT_ARRAY("Signature", c._cert_signature.value,
		    (uint32_t)c._cert_signature.len);

	/*get the CA's public key*/
	uint8_t *root_pk = NULL;
	uint32_t root_pk_len = 0;
	TRY(ca_pk_get(cred_array, cred_num, c._cert_issuer.value, &root_pk,
		      &root_pk_len));

	/*verify the certificates signature*/
	TRY(verify((enum sign_alg)c._cert_issuer_signature_algorithm, root_pk,
		   root_pk_len, cert,
		   cert_len - 2 - (uint32_t)c._cert_signature.len,
		   c._cert_signature.value, (uint32_t)c._cert_signature.len,
		   verified));

	TRY(_memcpy_s(pk, *pk_len, c._cert_pk.value, (uint32_t)c._cert_pk.len));
	*pk_len = (uint32_t)c._cert_pk.len;

	return ok;
}

enum err cert_x509_verify(const uint8_t *cert, uint32_t cert_len,
			  const struct other_party_cred *cred_array,
			  uint16_t cred_num, uint8_t *pk, uint32_t *pk_len,
			  bool *verified)
{
#ifdef MBEDTLS

	PRINT_MSG("Start parsing an ASN.1 certificate\n");

	mbedtls_x509_crt m_cert;
	mbedtls_x509_crt_init(&m_cert);

	/* parse the certificate */
	TRY_EXPECT(mbedtls_x509_crt_parse_der_nocopy(&m_cert, cert, cert_len),
		   0);

	/* some raw data from certificate */
	PRINT_ARRAY("cert.serial", m_cert.serial.p,
		    (uint32_t)m_cert.serial.len);
	PRINT_ARRAY("cert.issuer_raw", m_cert.issuer_raw.p,
		    (uint32_t)m_cert.issuer_raw.len);

	/* write details about the issuer */
	/* and find CN (Common Name), further referred to as "issuer_id" */
	const mbedtls_x509_name *p = &m_cert.issuer;

#ifdef DEBUG_PRINT
	const char *short_name;
#endif
	const mbedtls_asn1_buf *issuer_id = NULL;
	while (p) {
#ifdef DEBUG_PRINT
		mbedtls_oid_get_attr_short_name(&p->oid, &short_name);
		PRINTF("        %s: %.*s\n", short_name, (int)p->val.len,
		       p->val.p);
#endif
		if (0 == MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &p->oid)) {
			issuer_id = &p->val;
		}
		p = p->next;
	};

	PRINT_ARRAY("cert issuer_id", issuer_id->p, (uint32_t)issuer_id->len);

	enum sign_alg sign_alg;

	/* make sure it is ECDSA */
	if (MBEDTLS_PK_ECDSA == m_cert.sig_pk) {
		sign_alg = ES256;
	} else {
		mbedtls_x509_crt_free(&m_cert);
		return unsupported_signature_algorithm;
	}

	/* check hash algorithm and init signature buffer */
	const mbedtls_md_info_t *md_info =
		mbedtls_md_info_from_type(m_cert.sig_md);
	if (NULL == md_info) {
		PRINTF("mbedtls_md_info_from_type(%d) : not found\n",
		       m_cert.sig_md);
		mbedtls_x509_crt_free(&m_cert);
		return unsupported_signature_algorithm;
	}
	int hash_len = mbedtls_md_get_size(md_info);

	size_t sig_len = get_signature_len(sign_alg);
	TRY(check_buffer_size(SIGNATURE_DEFAULT_SIZE, (uint32_t)sig_len));
	uint8_t sig[SIGNATURE_DEFAULT_SIZE];

	/* get the public key of the CA */
	uint8_t *root_pk;
	uint32_t root_pk_len;
	TRY(ca_pk_get(cred_array, cred_num, issuer_id->p, &root_pk,
		      &root_pk_len));

	/* deserialize signature from ASN.1 to raw concatenation of (R, S) */
	{
		uint8_t *pp = m_cert.sig.p;
		struct deser_sign_ctx_s deser_sign_ctx;
		deser_sign_ctx_init(&deser_sign_ctx, sig, sig + sig_len,
				    hash_len);
		mbedtls_asn1_traverse_sequence_of(&pp, pp + m_cert.sig.len, 0,
						  0, 0, 0, deser_sign_cb,
						  &deser_sign_ctx);
		PRINT_ARRAY("Certificate signature", sig, (uint32_t)sig_len);
	}

	/*verify the certificates signature*/
	TRY(verify(sign_alg, root_pk, root_pk_len, m_cert.tbs.p,
		   (uint32_t)m_cert.tbs.len, sig, (uint32_t)sig_len, verified));

	/* export the public key from certificate */
	{
		uint8_t *cpk = NULL;
		size_t cpk_len = 0;
		uint8_t *pp = m_cert.pk_raw.p;
		mbedtls_asn1_traverse_sequence_of(&pp, pp + m_cert.pk_raw.len,
						  0, 0, 0, 0, find_pk_cb, &cpk);
		if (cpk) {
			if (*cpk == 0) {
				++cpk;
			}
			cpk_len = m_cert.pk_raw.len -
				  (size_t)(cpk - m_cert.pk_raw.p);
		}
		TRY(_memcpy_s(pk, *pk_len, cpk, (uint32_t)cpk_len));
		*pk_len = (uint32_t)cpk_len;
		PRINT_ARRAY("pk from cert", pk, *pk_len);
	}

	/* cleanup */
	mbedtls_x509_crt_free(&m_cert);

	return ok;

#else /* MBEDTLS */

	const uint8_t *tbs_start = cert;
	const uint8_t *tbs_end = &cert[ cert_len ];
	size_t sig_len = 0;
	uint8_t sig[SIGNATURE_DEFAULT_SIZE];

	enum err rv = certificate_authentication_failed;

	/* Crude way to get TBSCertificate address, public key and signature */
	do {
		const uint8_t *cursor = cert;
		const uint8_t *end = &cert[ cert_len ];
		size_t len;

		/* Get first tag, which should be first sequence */
		EXPECTO_TAG( ASN1_SEQUENCE, cursor, len );

		/* Get second, inner tag, which should be TBSCertificate */
		tbs_start = cursor;
		EXPECTO_TAG( ASN1_SEQUENCE, cursor, len );
		tbs_end = cursor + len;

		/* Iterate over 6 elements to get to public key according to X.509 schema */
		for (size_t iter = 0; iter < 6; iter++) {
			/* TAG shall not be parsed as it is not used */
			cursor++;
			/* Get section length */
			mbedtls_asn1_get_len(&cursor, end, &len);
			cursor += len;
		}

		/* Here should be 7th element with information about key */
		EXPECTO_TAG( ASN1_SEQUENCE, cursor, len );

		/* Here should be element with information about type of key */
		EXPECTO_TAG( ASN1_SEQUENCE, cursor, len );
		/* This section is being skippped */
		cursor += len;

		/* Expected BIT STRING containing public key */
		EXPECTO_TAG( ASN1_BIT_STRING, cursor, len );

		/* Now cursor points to public key */
		_memcpy_s(pk, *pk_len, cursor, (uint32_t)len);
		*pk_len = (uint32_t)len;
		PRINT_ARRAY("pk from cert", pk, *pk_len);

		/* We can skip whole TBSCertificate */
		cursor = tbs_end;

		/* Here should be information about signature algorithm */
		EXPECTO_TAG( ASN1_SEQUENCE, cursor, len );
		/* We can skip algorithm info length */
		cursor += len;

		/* Expected BIT STRING containing signature */
		EXPECTO_TAG( ASN1_BIT_STRING, cursor, len );
		// cursor++;

		EXPECTO_TAG( ASN1_SEQUENCE, cursor, len );
		
		EXPECTO_TAG( ASN1_INTEGER, cursor, len );

		TRY_EXPECT( (cursor + len) <= end, 1 );
		_memcpy_s(sig, SIGNATURE_DEFAULT_SIZE, cursor, (uint32_t)len);
		sig_len = len;
		cursor += len;
		PRINT_ARRAY("Certificate signature - part1", sig, (uint32_t)sig_len);

		EXPECTO_TAG( ASN1_INTEGER, cursor, len );
		TRY_EXPECT( (cursor + len) <= end, 1 );
		_memcpy_s(sig + sig_len, (uint32_t)(SIGNATURE_DEFAULT_SIZE - sig_len), cursor, (uint32_t)len);
		sig_len += len;
		rv = ok;
		PRINT_ARRAY("Certificate signature", sig, (uint32_t)sig_len);

	} while (0);

	uint8_t *root_pk;
	uint32_t root_pk_len = 0;

	if (ok == rv)
	{
		for (size_t iter = 0; iter < cred_num; iter++)
		{
			/* Issuer will not be read from certificate, so no identification is possible within public keys array. */
			ca_pk_get(cred_array + iter, 1, NULL, &root_pk, &root_pk_len);
			PRINT_ARRAY("pk from cred_list", root_pk, root_pk_len);
			rv = verify(ES256, root_pk, root_pk_len, tbs_start, (uint32_t)(tbs_end - tbs_start), sig, (uint32_t)sig_len, verified);
		}
	}
	return rv;

#endif /* MBEDTLS */
}
