import pytest
import json
import requests
import subprocess
import time


class Leshan:
    def __init__(self, url):
        self.api_url = url
        self.timeout = 60
        self.format = "TLV"
        # self.format = "SENML_CBOR"

        def __del__(self):
            # Allow device to receive re-registration message and quit
            time.sleep(1)

    @staticmethod
    def handle_response(resp):
        """Generic response handler for all queries"""
        if resp.status_code >= 300 or resp.status_code < 200:
            raise RuntimeError(f'Error {resp.status_code}: {resp.text}')
        obj = json.loads(resp.text)
        return obj

    def set_format(self, format):
        self.format = format

    def set_timeout(self, timeout):
        self.timeout = timeout

    def get(self, path):
        """Send HTTP GET query"""
        resp = requests.get(f"{self.api_url}{path}?timeout={self.timeout}&format={self.format}")
        return Leshan.handle_response(resp)

    def put(self, path, data):
        resp = requests.put(f"{self.api_url}{path}?timeout={self.timeout}&format={self.format}", data=data, headers={'content-type': 'application/json'})
        return Leshan.handle_response(resp)

    def post(self, path):
        resp = requests.post(f"{self.api_url}{path}")
        return Leshan.handle_response(resp)

    def execute(self, path):
        return self.post(path)
