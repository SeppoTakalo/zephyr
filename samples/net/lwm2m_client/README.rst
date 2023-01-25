#####################################
Twister2 evaluation for LwM2M testing
#####################################

This repository/branch/fork is a temporary evaluation using Pytest + Twister2 framework for testing
LwM2M stack in emulated environment.

*************************
Required files for Pytest
*************************

* `testspec.yaml` is similar like on previous Twister framework, but only parsed in Twister2.
* `samples.yaml` is removed, but this is unrelated change, just to prevent the build.
* `lwm2m_test.py` is a testcase written like Pytest tests.
* `leshan.py` is a helper to talk to local Leshan instance's REST API.
* `pytest.ini` Just for testing, I'm tweaking Pytest so I can see console output.

*******
Running
*******

I have a Leshan instance running in localhost, answering port HTTP://localhost:8080.
It is reachable through `coap://192.0.2.2:5683` and `coaps://192.0.2.2:5684`

::
   ❯ pytest --platform=qemu_cortex_m3 -s --clear=no
   Keeping previous artifacts untouched
   2023-01-27 13:28:39,992:INFO:twister2.platform_specification: Reading platform configuration files under /home/seta/src/ncs/zephyr/boards
   2023-01-27 13:28:40,764:INFO:twister2.platform_specification: Reading platform configuration files under /home/seta/src/ncs/zephyr/scripts/pylib/twister/boards
   =============================================================== test session starts ===============================================================
   platform linux -- Python 3.8.10, pytest-7.2.1, pluggy-1.0.0
   rootdir: /home/seta/src/ncs/zephyr/samples/net/lwm2m_client, configfile: pytest.ini
   plugins: twister-0.0.1, subtests-0.9.0
   collected 1 item

   lwm2m_test.py::test_lwm2m_registration[qemu_cortex_m3:sample.net.lwm2m] 2023-01-27 13:28:41,692:INFO:twister2.builder.build_manager: Already build in /home/seta/src/ncs/zephyr/samples/net/lwm2m_client/twister-out/qemu_cortex_m3/sample.net.lwm2m
   2023-01-27 13:28:41,693:INFO:twister2.device.qemu_adapter: Running command: /home/seta/.virtualenvs/zephyr/bin/west build -d /home/seta/src/ncs/zephyr/samples/net/lwm2m_client/twister-out/qemu_cortex_m3/sample.net.lwm2m -t run
   uart:~$ shell colors off
   *** Booting Zephyr OS build zephyr-v3.2.0-4054-g9fbfdf2b00e2 ***
   [00:00:00.010,000] <dbg> net_lwm2m_engine: lwm2m_engine_init: LWM2M engine socket receive thread started
   [00:00:00.010,000] <dbg> net_lwm2m_obj_security: security_create: Create LWM2M security instance: 0
   [00:00:00.010,000] <dbg> net_lwm2m_obj_server: server_create: Create LWM2M server instance: 0
   [00:00:00.010,000] <dbg> net_lwm2m_obj_device: device_create: Create LWM2M device instance: 0
   [00:00:00.010,000] <dbg> net_lwm2m_obj_firmware: firmware_create: Create LWM2M firmware instance: 0

   ........ clip ....

   [00:00:05.120,000] <inf> net_lwm2m_rd_client: Update Done
   PASSED2023-01-27 13:28:51,429:INFO:twister2.device.qemu_adapter: Running simulation stopped interrupted by user

   =============================================================== 1 passed in 10.65s ================================================================
