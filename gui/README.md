GUI
===

Status: does a few things

Building
--------

Assuming you have nodejs installed,

```shell
$ make
```

should spin up a local http server on port 8000.

Commandline Tools
-----------------

- `scripts/send`

Sends stdin to their servers as a string literal, and expects a string literal from their server, and prints it on stdout.

Development
-----------

Other make targets are:

```shell
make watch # build js whenever ts source changes
make build # build once
make check # typecheck js whenever ts source changes
make test  # run tests
```
