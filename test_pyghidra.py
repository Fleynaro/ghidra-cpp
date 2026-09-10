import pyghidra

print("Starting PyGhidra...")

pyghidra.start()

print("PyGhidra started!")

from ghidra.framework import Application

print("Ghidra version:", Application.getApplicationVersion())
print("Ghidra home:", Application.getInstallationDirectory())

print("SUCCESS")
