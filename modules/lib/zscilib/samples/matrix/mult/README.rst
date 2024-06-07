.. _zscilib-mtx-mult-sample:

Matrix Multiply
###############

Overview
********

This sample application shows how to multiple two matrices together using
zscilib. The results will be sent to the console for verification.

Requirements
************

Depending on the device you are testing with, you may need to enable or
disable the ``CONFIG_STDOUT_CONSOLE`` flag.

It should be set to ``y`` when using the emulator, and to ``n`` when running on
real hardware.

Building and Running
********************

To run this sample on the **Cortex M3 emulator**, run the following commands
which will compile the application, run it on the emulator, and output
the result to the console:

.. code-block:: console

    $ rm -rf build
    $ west build -b qemu_cortex_m3 samples/matrix/mult -t run

To run the application on real HW, typically outputting the results to the
serial port, you can try a variant of the following, adjusting ``-b BOARD``
as appropriate. The **nRF52840 PCA10056** from Nordic Semiconductors is used
below:

.. code-block:: console

    $ rm -rf build
    $ west build -b nrf52840_pca10056 samples/matrix/mult
    $ west flash
