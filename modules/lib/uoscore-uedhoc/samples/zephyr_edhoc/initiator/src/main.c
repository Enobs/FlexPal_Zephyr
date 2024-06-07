/*
   Copyright (c) 2021 Fraunhofer AISEC. See the COPYRIGHT
   file at the top-level directory of this distribution.

   Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
   http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
   <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
   option. This file may not be copied, modified, or distributed
   except according to those terms.
*/

#include <stdio.h>
#include <zephyr/net/coap.h>

#include "edhoc.h"
#include "sock.h"
#include "edhoc_test_vectors_p256_v16.h"

/**
 * @brief	Initializes sockets for CoAP client.
 * @param
 * @retval	error code
 */
static int start_coap_client(int *sockfd)
{
	struct sockaddr_in6 servaddr;
	//const char IPV6_SERVADDR[] = { "::1" };
	const char IPV6_SERVADDR[] = { "2001:db8::2" };
	int r = ipv6_sock_init(SOCK_CLIENT, IPV6_SERVADDR, &servaddr,
			       sizeof(servaddr), sockfd);
	if (r < 0) {
		printf("error during socket initialization (error code: %d)",
		       r);
		return -1;
	}
	return 0;
}

/**
 * @brief	Callback function called inside the frontend when data needs to 
 * 		be send over the network. We use here CoAP as transport 
 * @param	data pointer to the data that needs to be send
 * @param	data_len lenhgt of the data in bytes
 */
enum err tx(void *sock, uint8_t *data, uint32_t data_len)
{
	/* Initialize the CoAP message */
	char *path = ".well-known/edhoc";
	struct coap_packet request;
	uint8_t _data[1000];

	TRY_EXPECT(coap_packet_init(&request, _data, sizeof(_data), 1,
				    COAP_TYPE_CON, 8, coap_next_token(),
				    COAP_METHOD_POST, coap_next_id()),
		   0);

	/* Append options */
	TRY_EXPECT(coap_packet_append_option(&request, URI_PATH,
					     path, strlen(path)),
		   0);

	/* Append Payload marker if you are going to add payload */
	TRY_EXPECT(coap_packet_append_payload_marker(&request), 0);

	/* Append payload */
	TRY_EXPECT(coap_packet_append_payload(&request, data, data_len), 0);

	PRINT_ARRAY("CoAP packet", request.data, request.offset);
	ssize_t n = send(*((int *)sock), request.data, request.offset, 0);
	if (n < 0) {
		printf("send failed with error code: %d\n", n);
	} else {
		printf("%d bytes sent\n", n);
	}

	return ok;
}

/**
 * @brief	Callback function called inside the frontend when data needs to 
 * 		be received over the network. We use here CoAP as transport 
 * @param	data pointer to the data that needs to be received
 * @param	data_len lenhgt of the data in bytes
 */
enum err rx(void *sock, uint8_t *data, uint32_t *data_len)
{
	int n;
	char buffer[MAXLINE];
	struct coap_packet reply;
	const uint8_t *edhoc_data_p;
	uint16_t edhoc_data_len;

	/* receive */
	n = recv(*((int *)sock), (char *)buffer, MAXLINE, MSG_WAITALL);
	if (n < 0) {
		printf("recv error");
	}

	PRINT_ARRAY("received data", buffer, n);

	TRY_EXPECT(coap_packet_parse(&reply, buffer, n, NULL, 0), 0);

	edhoc_data_p = coap_packet_get_payload(&reply, &edhoc_data_len);

	PRINT_ARRAY("received EDHOC data", edhoc_data_p, edhoc_data_len);

	if (*data_len >= edhoc_data_len) {
		memcpy(data, edhoc_data_p, edhoc_data_len);
		*data_len = edhoc_data_len;
	} else {
		printf("insufficient space in buffer");
		return buffer_to_small;
	}

	return ok;
}

void main(void)
{
	int32_t s = 5000;
	printf("sleep for %d msecond after connection in order to have time to start wireshark on bt0\n", s);
	k_msleep(s);


	int sockfd;
	uint8_t prk_exporter[32];
	uint8_t oscore_master_secret[16];
	uint8_t oscore_master_salt[8];

	/* edhoc declarations */
	uint8_t PRK_out[PRK_DEFAULT_SIZE];
	uint8_t err_msg[ERR_MSG_DEFAULT_SIZE];
	uint32_t err_msg_len = sizeof(err_msg);

	/* test vector inputs */
	const uint8_t TEST_VEC_NUM = 2;
	uint16_t cred_num = 1;
	struct other_party_cred cred_r;
	struct edhoc_initiator_context c_i;

	uint8_t vec_num_i = TEST_VEC_NUM - 1;

	c_i.msg4 = true;
	c_i.sock = &sockfd;
	c_i.c_i.len = test_vectors[vec_num_i].c_i_len;
	c_i.c_i.ptr = (uint8_t *)test_vectors[vec_num_i].c_i;
	c_i.method = (enum method_type) * test_vectors[vec_num_i].method;
	c_i.suites_i.len = test_vectors[vec_num_i].SUITES_I_len;
	c_i.suites_i.ptr = (uint8_t *)test_vectors[vec_num_i].SUITES_I;
	c_i.ead_1.len = test_vectors[vec_num_i].ead_1_len;
	c_i.ead_1.ptr = (uint8_t *)test_vectors[vec_num_i].ead_1;
	c_i.ead_3.len = test_vectors[vec_num_i].ead_3_len;
	c_i.ead_3.ptr = (uint8_t *)test_vectors[vec_num_i].ead_3;
	c_i.id_cred_i.len = test_vectors[vec_num_i].id_cred_i_len;
	c_i.id_cred_i.ptr = (uint8_t *)test_vectors[vec_num_i].id_cred_i;
	c_i.cred_i.len = test_vectors[vec_num_i].cred_i_len;
	c_i.cred_i.ptr = (uint8_t *)test_vectors[vec_num_i].cred_i;
	c_i.g_x.len = test_vectors[vec_num_i].g_x_raw_len;
	c_i.g_x.ptr = (uint8_t *)test_vectors[vec_num_i].g_x_raw;
	c_i.x.len = test_vectors[vec_num_i].x_raw_len;
	c_i.x.ptr = (uint8_t *)test_vectors[vec_num_i].x_raw;
	c_i.g_i.len = test_vectors[vec_num_i].g_i_raw_len;
	c_i.g_i.ptr = (uint8_t *)test_vectors[vec_num_i].g_i_raw;
	c_i.i.len = test_vectors[vec_num_i].i_raw_len;
	c_i.i.ptr = (uint8_t *)test_vectors[vec_num_i].i_raw;
	c_i.sk_i.len = test_vectors[vec_num_i].sk_i_raw_len;
	c_i.sk_i.ptr = (uint8_t *)test_vectors[vec_num_i].sk_i_raw;
	c_i.pk_i.len = test_vectors[vec_num_i].pk_i_raw_len;
	c_i.pk_i.ptr = (uint8_t *)test_vectors[vec_num_i].pk_i_raw;

	cred_r.id_cred.len = test_vectors[vec_num_i].id_cred_r_len;
	cred_r.id_cred.ptr = (uint8_t *)test_vectors[vec_num_i].id_cred_r;
	cred_r.cred.len = test_vectors[vec_num_i].cred_r_len;
	cred_r.cred.ptr = (uint8_t *)test_vectors[vec_num_i].cred_r;
	cred_r.g.len = test_vectors[vec_num_i].g_r_raw_len;
	cred_r.g.ptr = (uint8_t *)test_vectors[vec_num_i].g_r_raw;
	cred_r.pk.len = test_vectors[vec_num_i].pk_r_raw_len;
	cred_r.pk.ptr = (uint8_t *)test_vectors[vec_num_i].pk_r_raw;
	cred_r.ca.len = test_vectors[vec_num_i].ca_r_len;
	cred_r.ca.ptr = (uint8_t *)test_vectors[vec_num_i].ca_r;
	cred_r.ca_pk.len = test_vectors[vec_num_i].ca_r_pk_len;
	cred_r.ca_pk.ptr = (uint8_t *)test_vectors[vec_num_i].ca_r_pk;

	start_coap_client(&sockfd);
	edhoc_initiator_run(&c_i, &cred_r, cred_num, err_msg, &err_msg_len,
				NULL, NULL, NULL, NULL, PRK_out,
			    sizeof(PRK_out), tx, rx);

	PRINT_ARRAY("PRK_out", PRK_out, sizeof(PRK_out));

	prk_out2exporter(SHA_256, PRK_out, sizeof(PRK_out), prk_exporter);
	PRINT_ARRAY("prk_exporter", prk_exporter, sizeof(prk_exporter));

	edhoc_exporter(SHA_256, OSCORE_MASTER_SECRET, prk_exporter,
		       sizeof(prk_exporter), oscore_master_secret,
		       sizeof(oscore_master_secret));
	PRINT_ARRAY("OSCORE Master Secret", oscore_master_secret,
		    sizeof(oscore_master_secret));

	edhoc_exporter(SHA_256, OSCORE_MASTER_SALT, prk_exporter,
		       sizeof(prk_exporter), oscore_master_salt,
		       sizeof(oscore_master_salt));
	PRINT_ARRAY("OSCORE Master Salt", oscore_master_salt,
		    sizeof(oscore_master_salt));

	close(sockfd);
}
