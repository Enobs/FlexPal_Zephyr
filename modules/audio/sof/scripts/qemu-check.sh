#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2018 Intel Corporation. All rights reserved.
set -e

SUPPORTED_PLATFORMS=(imx8 imx8x imx8m)

SOF_DIR=$(cd "$(dirname "$0")" && cd .. && pwd)

: "${SOF_BUILDS:=${SOF_DIR}}"

rm -f dump-*.txt

die()
{
	>&2 printf '%s ERROR: ' "$0"
	# We want die() to be usable exactly like printf
	# shellcheck disable=SC2059
	>&2 printf "$@"
	exit 1
}

print_usage()
{
	cat <<EOF
usage: qemu-check.sh [ platform(s) ]
	Supported platforms are ${SUPPORTED_PLATFORMS[*]}
	Runs all supported platforms by default.
EOF
}

find_qemu_xtensa()
{
	local xhs=xtensa-host.sh
	for d in . ../qemu* qemu*; do
		if test -e "$d"/$xhs; then
			printf '%s' "$d"/$xhs;
			return
		fi
	done
	die '%s not found\n' $xhs
}


while getopts "" OPTION; do
	case "$OPTION" in
		*) print_usage; exit 1 ;;
	esac
done
shift $((OPTIND-1))

PLATFORMS=()
if [ "$#" -eq 0 ]; then
	PLATFORMS=("${SUPPORTED_PLATFORMS[@]}")
else
	for arg in "$@"; do
		platform=unknown
		for sp in "${SUPPORTED_PLATFORMS[@]}"; do
			if [ x"$sp" = x"$arg" ]; then
				PLATFORMS=("${PLATFORMS[@]}" "$sp")
				platform=$sp
				shift
				break
			fi
		done
		if [ "$platform" = "unknown" ]; then
			echo "Error: Unknown platform specified: $arg"
			echo "Supported platforms: ${SUPPORTED_PLATFORMS[*]}"
			exit 1
		fi
	done
fi

for platform in "${PLATFORMS[@]}"
do
	FWNAME="sof-$platform.ri"
	PLATFORM=$platform
	# reset variable to avoid issue in random order
	ROM=''
	OUTBOX_OFFSET=''

	has_rom=false
	case "$platform" in
		imx8 | imx8x | imx8m)
		# This READY_IPC value comes from:
		#
		#	/* Disable interrupt from MU:
		#	 * GP #0 for Host -> DSP message notification
		#	 * GP #1 for DSP -> Host message confirmation
		#	 * GP #2 and #3 not used
		#	 */
		#	imx_mu_xcr_rmw(IMX_MU_VERSION, IMX_MU_GIER, 0,
		#				   IMX_MU_xCR_GIEn(IMX_MU_VERSION, 0) |
		#				   IMX_MU_xCR_GIEn(IMX_MU_VERSION, 1) |
		#				   IMX_MU_xCR_GIEn(IMX_MU_VERSION, 2) |
		#				   IMX_MU_xCR_GIEn(IMX_MU_VERSION, 3));
		#	/* Clear all pending interrupts from MU */
		#	imx_mu_write(IMX_MU_xSR_GIPn(IMX_MU_VERSION, 0) |
		#			     IMX_MU_xSR_GIPn(IMX_MU_VERSION, 1) |
		#			     IMX_MU_xSR_GIPn(IMX_MU_VERSION, 2) |
		#			     IMX_MU_xSR_GIPn(IMX_MU_VERSION, 3),
		#			     IMX_MU_xSR(IMX_MU_VERSION, IMX_MU_GSR));
		#	/* Enable GP #0 and #1 for Host -> DSP and
		#	 * DSP -> Host message notification
		#	 */
		#	imx_mu_xcr_rmw(IMX_MU_VERSION, IMX_MU_GIER,
		#				   IMX_MU_xCR_GIEn(IMX_MU_VERSION, 0) |
		#				   IMX_MU_xCR_GIEn(IMX_MU_VERSION, 1), 0);
		#	/* Now interrupt host to tell it we are done booting */
		#	imx_mu_xcr_rmw(IMX_MU_VERSION, IMX_MU_GCR,
		#				   IMX_MU_xCR_GIRn(IMX_MU_VERSION, 1), 0);
		# So, "00 00 00 f0 00 00 04 c0" is the MU's xSR and xCR registers:
		#    xSR: F0000000 and xCR: C00400000
			READY_IPC="00 00 00 f0 00 00 04 c0"
			SHM_IPC_REG=qemu-bridge-mu-io
			SHM_MBOX=qemu-bridge-mbox-io
			;;
	esac

	if $has_rom; then
		ROM=(-r "${SOF_BUILDS}/build_${platform}_gcc/src/arch/xtensa/rom-$platform.bin")
	fi

        xtensa_host_sh=$(find_qemu_xtensa)

	ret=0
	( set -x

	${xtensa_host_sh} "$PLATFORM" -k \
	   "${SOF_BUILDS}"/build_"${platform}"_gcc/src/arch/xtensa/"$FWNAME" \
                "${ROM[@]}" \
		-o 2.0 "${SOF_BUILDS}"/dump-"$platform".txt
	# dump log into sof.git incase running in docker
	) || ret=$? # this defeats set -e
	# See 'man timeout'
	test $ret = 124 || die 'qemu failed before we stopped it!\n'

	# use regular expression to match the SHM IPC REG file name, one of:
	# /dev/shm/qemu-bridge-{shim,ipc,dsp}-io
	SHM_IPC_REG_FILE=$(ls /dev/shm/ | grep -E $SHM_IPC_REG) || true

	test -e /dev/shm/"${SHM_IPC_REG_FILE}" ||
	    die '%s: %s not found in /dev/shm/\n' "$0" "$SHM_IPC_REG"

	# Check if ready ipc header is in the ipc regs doorbell
	# registers or shared memory for older platforms.
	# See:
	# - 'adsp_reg_space' in qemu/
	# - shim_write() or ipc_write() in sof/
	IPC_REG=$(hexdump -C /dev/shm/"$SHM_IPC_REG_FILE" |
		      grep "$READY_IPC") || true

	# Check if ready ipc message is in the mbox-io / sram-mem
	# See "struct sof_ipc_fw_ready" and
	# objdump -s -j .fw_ready -s build/sof
	READY_MSG="6c 00 00 00 00 00 00 70"
	IPC_MSG=$(hexdump -C /dev/shm/$SHM_MBOX | grep -A 4 "$READY_MSG" |
		      grep -A 4 "$OUTBOX_OFFSET") || true

	if [ "$IPC_REG" ]; then
		echo "ipc reg dump:"
		# directly output the log to be nice to look
		hexdump -C /dev/shm/"$SHM_IPC_REG_FILE" | grep "$READY_IPC"
		if [ "$IPC_MSG" ]; then
			echo "ipc message dump:"
			# directly output the log to be nice to look
			hexdump -C /dev/shm/$SHM_MBOX | grep -A 4 "$READY_MSG" | grep -A 4 "$OUTBOX_OFFSET"
		else
			echo "Error mailbox failed"
		fi
	else
		echo "Error ipc reg failed"
	fi

	if [[ "$IPC_REG" && "$IPC_MSG" ]]; then
		echo "Boot success";
	else
		echo "Error boot failed"
		tail -n 50 "${SOF_BUILDS}"/dump-"$platform".txt
		exit 2;
	fi
done
