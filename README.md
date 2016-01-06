# Parallelism challenge of the 2015 edition of the CS Games

## Folders structure
- *data*: Test and evaluation data to be acquired by the server at start up.
- *docs*: English and French version of the original challenge description.
- *old*: Old version of the challenge. For historic purposes only.
- *src*: Challenge to be completed.

## Requirements
To compile, you need the following to be installed on your system
- Boost
- CMake
- Python 2.7

## Compiling
To compile the source code, you first need to create a build directory like so:
```bash
mkdir cs_build
```
Please don't create the build directory inside the source directory. CMake generates a lot of files for its uses and it will make a mess inside your source directory.

Afterwards, you'll need to step into the build directory you just created.
```bash
cd cs_build
```

Here, you'll be able to generate the MakeFiles by invoking CMake.
```bash
cmake <path/to/source/directory>
```

Finally, you can compile by invoking make inside the build directory.

## Running
Once compilation is done, you can run the program by invoking the start_comp.py script. Note that this script takes an argument that will determine the size of the datasets to use (Test mode uses small datasets while Eval mode uses large datasets). 
```bash
python start_comp.py -m (Test | Eval)
```

