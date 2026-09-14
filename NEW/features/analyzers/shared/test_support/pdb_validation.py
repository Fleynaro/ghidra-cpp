"""Validate the CodeView identity shared by PE/PDB analyzer fixtures."""

from __future__ import annotations

from pathlib import Path


def _normalise_guid(value) -> str:
    """Return a GUID in the comparison form used by the PDB reader."""
    return str(value).strip().replace("{", "").replace("}", "").replace("-", "").upper()


def validate_pdb_match(program, pdb_path: Path) -> None:
    """Require a readable PDB whose identity matches the imported PE CodeView record."""
    from ghidra.app.util.bin.format.pdb import PdbParserConstants
    from ghidra.app.util.bin.format.pdb2.pdbreader import PdbParser, PdbReaderOptions
    from ghidra.program.model.listing import Program
    from ghidra.util.task import TaskMonitor
    from java.io import File

    if not pdb_path.is_file():
        raise FileNotFoundError(f"Matching PDB does not exist: {pdb_path}")

    program_info = program.getOptions(Program.PROGRAM_INFO)
    embedded_name = str(program_info.getString(PdbParserConstants.PDB_FILE, "")).replace("\\", "/")
    if embedded_name and Path(embedded_name).name.casefold() != pdb_path.name.casefold():
        raise RuntimeError(f"PE references PDB {Path(embedded_name).name!r}, but the runner selected {pdb_path.name!r}")

    expected_guid = str(program_info.getString(PdbParserConstants.PDB_GUID, ""))
    expected_signature = str(program_info.getString(PdbParserConstants.PDB_SIGNATURE, ""))
    expected_age_text = str(program_info.getString(PdbParserConstants.PDB_AGE, ""))
    if not expected_guid and not expected_signature:
        raise RuntimeError("Imported PE has no CodeView GUID or signature to identify its PDB")
    try:
        expected_age = int(expected_age_text, 16)
    except ValueError as error:
        raise RuntimeError(f"Imported PE has an invalid CodeView PDB age: {expected_age_text!r}") from error

    pdb = PdbParser.parse(File(str(pdb_path)), PdbReaderOptions(), TaskMonitor.DUMMY)
    try:
        identifiers = pdb.getIdentifiers()
        actual_age = int(identifiers.getAge()) & 0xFFFFFFFF
        if actual_age != expected_age:
            raise RuntimeError(f"PE/PDB age mismatch: PE={expected_age:X}, PDB={actual_age:X} ({pdb_path})")
        actual_guid = identifiers.getGuid()
        if expected_guid:
            if actual_guid is None or _normalise_guid(actual_guid) != _normalise_guid(expected_guid):
                raise RuntimeError(f"PE/PDB GUID mismatch: PE={expected_guid}, PDB={actual_guid} ({pdb_path})")
        elif int(identifiers.getSignature()) & 0xFFFFFFFF != int(expected_signature, 16):
            raise RuntimeError(f"PE/PDB signature mismatch: PE={expected_signature}, PDB={int(identifiers.getSignature()) & 0xFFFFFFFF:08X} ({pdb_path})")
    finally:
        pdb.close()
