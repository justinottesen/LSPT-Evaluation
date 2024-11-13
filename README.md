# Evaluation
Hello World!

Description TBD...

## Build Instructions

### Upon Cloning the Repository

Begin by running the `setup_repo.sh` script. This will do more in the future. Currently it does the following:

1. Clones the googletest repository if it is not present
2. Builds the libraries from the googletest source

### Build & Run Instructions

#### Evaluation

Enter the evaluation directory, and run make. This will create a binary titled `evaluation` in the `evaluation/bin` directory which can be executed:

```
cd evaluatoin
make
./bin/evaluation
```

Alternatively, the `make run` shortcut will also do this

#### Evaltool

Currently this is python, and it is set up with the shebang, so all that is necessary should be `./evaltool/evaltool.py <arguments>`. This may change if we choose to use C++

#### Unit Tests

Enter the `unittest` directory, and run `make`. By default, this will make all unit tests, linking with the google test libraries built by `setup_repo.sh`.

**IF YOU GET ERRORS HERE, IT IS PROBABLY BECAUSE YOU DIDN'T RUN THE SETUP SCRIPT**

Or maybe I set it up wrong, still a work in progress

#### Component Tests

Enter the `componenttest` directory and run `pytest`. This should run everything.

## Helpful Resources

- [GoogleTest User's Guide](https://google.github.io/googletest/)
- [Pytest Documentation](https://docs.pytest.org/en/stable/index.html)
- [Makefile Tutorial](https://makefiletutorial.com/)
