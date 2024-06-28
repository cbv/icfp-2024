Frontend Template
=================

This repository is meant as a convenient starting point that bakes in
a few of my favorite libraries for frontend development.

Building
--------

Assuming you have nodejs installed,

```shell
$ make
```

should spin up a local http server on port 8000.

Development
-----------

Other make targets are:

```shell
make watch # build js whenever ts source changes
make build # build once
make check # typecheck js whenever ts source changes
make test  # run tests
```
