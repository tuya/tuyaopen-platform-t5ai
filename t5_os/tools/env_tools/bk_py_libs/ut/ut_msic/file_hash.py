import hashlib
from pathlib import Path


def get_file_md5sum(file: Path):
    md5_hash = hashlib.md5()  # noqa S324
    bin_content = file.read_bytes()

    md5_hash.update(bin_content)
    return md5_hash.hexdigest()
