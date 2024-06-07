.. _zscilib-benchmark-sample:

Orientation API Test
####################

Overview
********

This sample demonstrates how to use the orientation API to take raw
accel, mag and gyro data, calibrate and feed it into a sensor fusion
algorithm.

Building and Running
********************

To run in QEMU:

.. code-block:: console

    $ west build -p auto -b mps2_an521 \
      modules/lib/zscilib/samples/orientation/apitest/ -t run

To run the application on real HW, typically outputting the results to the
serial port, you can try a variant of the following, adjusting ``-b``
as appropriate.

The **nRF52840 PCA10056** from Nordic Semiconductors is used below:

.. code-block:: console

    $ west build -b nrf52840_pca10056 \
      modules/lib/zscilib/samples/orientation/apitest/
    $ west flash
