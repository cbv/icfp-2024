#!/usr/bin/env python3

import os
import requests
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

API_KEY_PATH = "API_KEY"

def get_api_key():
    if not(os.path.isfile(API_KEY_PATH)):
        eprint("please download an API key from https://icfpcontest2024.github.io/team.html " +
               "and save it in a file named {}".format(API_KEY_PATH))
        exit(1)
    return open(API_KEY_PATH, "r").read().strip()

if __name__ == "__main__":
    api_key = get_api_key()
    headers = {'Authorization': 'Bearer {}'.format(api_key)}

    response = requests.get(
        "https://boundvariable.space/team",
        headers = headers)

    if response.status_code != 200:
        eprint("error. status = {}".format(response.status_code))
        eprint(response.content)
        exit(1)

    print("success!")
    print(response.text)
