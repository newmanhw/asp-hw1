# Assignment 1 - Newman Waters

## Required
- Ubuntu 24.04 LTS
- `g++` compiler
- `make` for building the project

## Setting up
Run `make` in the root directory of the project. `magic_transformer` is the primary file to be run, and it runs new processes (which are built as `transformerX`) within. 

To rebuild, it is simple enough to run `make clean`.

## Implementation
The transformers were designed in pure C. These transformers simply do text processessing (similar to a CSV), outputting different columns onto stdout and stderr. The top-level magic_transformer takes in ALL input, and then passes it through each processes and then decides where to send outputs at the very end once it has all been processed.

The magic transformer was made using C++.