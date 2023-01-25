import time
import re
import pytest
from leshan import Leshan

@pytest.fixture(scope="module")
def leshan():
    return Leshan("http://localhost:8080/api")

class Shell(object):
    def __init__(self, dut) -> None:
        self._dut = dut
        #dut._wait_for_fifo()
        #self.pipe = dut.iter_stdout
        self.connection = dut.connection
        #next(self.pipe)

    def find(self, regex):
        pattern = re.compile(regex)
        matched = False
        timeout = 5  # [sec]
        time_end = time.time() + timeout
        while time.time() < time_end:
            line = self.connection.readline()
            if line is None or len(line) == 0:
                continue
            print(line)
            m = pattern.match(line)
            if m is not None:
                return m

        return None

    def cmd(self, cmd):
        self.connection.write(b'log halt\n')
        self.connection.write(bytes(cmd + '\n', 'utf-8'))
        found = False
        answer = None
        timeout = 5  # [sec]
        time_end = time.time() + timeout
        while time.time() < time_end:
            line = self.connection.readline()
            if line is None or len(line) == 0:
                continue
            print(line)
            if found:
                if line.find('uart:~$') >= 0:
                    answer = ''
                else:
                    answer = line
                break
            if line.find(cmd) > 0:
                found = True
                # Next line will be the answer

        self.connection.write(b'log go\n')
        #print("answer: " + answer)
        return answer

    def write(self, line):
        self.connection.write(bytes(line + '\n', 'utf-8'))

@pytest.fixture(scope="function")
def shell(dut):
    return Shell(dut)


def verify_is_registered(shell):
    # Verify that we have registered
    # LightweightM2M-1.1-int-101
    assert shell.find('.*Registration Done')

def verify_registered_in_leshan(leshan, endpoint):
    resp = leshan.get(f'/clients/{endpoint}')
    assert resp


@pytest.mark.build_specification
def test_lwm2m_registration(builder, shell, leshan):
    #shell = Shell(dut)
    shell.write('shell colors off')
    # Verify that we start
    m = shell.find(r'.*net_lwm2m_rd_client: Start LWM2M Client: (\w*)')
    assert m
    endpoint = m.group(1)

    verify_is_registered(shell)
    verify_registered_in_leshan(leshan, endpoint)

    host = shell.cmd('lwm2m read 0/0/0 -s')
    assert host == 'coap://192.0.2.2:5683'

    # Test update
    shell.write('lwm2m update')
    assert shell.find('.*net_lwm2m_rd_client: Update Done')

    # Test Update when lifetime changes
    # LightweightM2M-1.1-int-102

    # Write a different lifetime to Server object
    # should cause update
    litetime = int(shell.cmd('lwm2m read 1/0/1 -u32'))
    lifetime = litetime + 10
    shell.write(f'lwm2m write 1/0/1 -u32 {lifetime}')
    assert shell.find('.*net_lwm2m_rd_client: Update Done')

    # Same using Leshan
    start_time = time.time() * 1000
    resp = leshan.put(f'/clients/{endpoint}/1/0/1', '{"id":1,"kind":"singleResource","value":"20","type":"integer"}')
    time.sleep(5)
    latest = leshan.get(f'/clients/{endpoint}')
    assert latest["lastUpdate"] > start_time
    assert latest["lastUpdate"] <= time.time()*1000
    assert latest["lifetime"] == 20

    #
    # LightweightM2M-1.1-int-104 – Registration Update Trigger
    #
    leshan.execute(f'/clients/{endpoint}/1/0/8')
    assert shell.find('.*net_lwm2m_rd_client: Update Done')
