import zstandard as zstd
from contextlib import contextmanager


class _ZReader:

  def __init__(self, z):
    self._z = z

  def read(self):
    res = []
    while True:
      chunk = self._z.read(16384)
      if chunk:
        res.append(chunk)
      else:
        return b''.join(res).decode()


@contextmanager
def zstd_open(path):
  with open(path, 'rb') as f:
    dctx = zstd.ZstdDecompressor()
    with dctx.stream_reader(f) as reader:
      yield _ZReader(reader)


def pretty_diff_changes(a, b):
  diffs = [
      '{}: {} -> {}'.format(k, a.get(k, 'no value'), b.get(k, 'no value'))
      for k in sorted(set(a.keys()) | set(b.keys()))
      if b.get(k, b) != a.get(k, a)
  ]
  return '\n'.join(diffs) if diffs else 'No changes'
