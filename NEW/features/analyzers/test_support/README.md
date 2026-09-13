# Analyzer Test Support

This directory contains the single shared Python helper implementation used by analyzer fixture scripts. [`evidence.py`](evidence.py) renders behavioral evidence, and [`pdb_validation.py`](pdb_validation.py) validates PDB matches. Fixture scripts add this directory to `sys.path` at runtime; the helpers must not be copied into individual `tests/data` directories.
