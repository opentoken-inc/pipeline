import sys
import os
import logging
from time import time
from json import dumps, loads
from hashlib import sha1
from uuid import getnode
from datetime import datetime, timedelta

logger = logging.getLogger('data_logger')
logging.basicConfig(level=logging.DEBUG)

_MAC_HEX = hex(getnode())[2:]


def mkdirs_exists_ok(path):
  try:
    os.makedirs(path)
  except OSError:
    if not os.path.isdir(path):
      raise


class DataLogger:

  def __init__(self, data_type_prefix,
               max_output_size_bytes=10000000,
               max_file_duration_seconds=180):
    self.working_dir = 'working'
    self.output_dir = 'compressing'
    self._data_type_prefix = data_type_prefix
    self._max_output_size = max_output_size_bytes
    self._max_file_duration = timedelta(seconds=max_file_duration_seconds)

    self.last_opened_time = None
    self.hasher = None
    self.f = None

    mkdirs_exists_ok(self.working_dir)
    mkdirs_exists_ok(self.output_dir)

  def log_line(self, line):
    self._maybe_roll_logfile()
    delimeted_line = line if line[-1] == '\n' else (line + '\n')
    new_data = delimeted_line.encode()
    self.hasher.update(new_data)
    self.f.write(new_data)
    self._maybe_roll_logfile()

  def log_json(self, obj):
    if isinstance(obj, (str, bytes)):
      obj = loads(obj)
    if isinstance(obj, list):
      obj = {'dList': obj}

    obj['logTime'] = str(time())
    self.log_line(dumps(obj, separators=(',', ':')))

  def _maybe_roll_logfile(self):
    if not self.f:
      self._roll_logfile()
      return

    bytes_written = self.f.tell()
    now = datetime.utcnow()
    if (bytes_written > self._max_output_size or
        now > self.last_opened_time + self._max_file_duration):
      self._roll_logfile()

  def _close_logfile(self):
    if self.f:
      old_path = self.f.name
      new_path = '{}_{}.json'.format(
          os.path.join(self.output_dir, os.path.basename(self.f.name)),
          self.hasher.digest()[:8].hex())
      self.f.close()
      self.f = None
      self.hasher = None
      os.rename(old_path, new_path)

  def _roll_logfile(self):
    self._close_logfile()

    now = datetime.utcnow()
    path = '{}/{}_{}_{}'.format(self.working_dir, self._data_type_prefix,
                                now.strftime('%Y_%m_%d_%H_%M_%S'), _MAC_HEX)
    self.last_opened_time = now
    self.hasher = sha1()
    self.f = open(path, 'wb')
    logger.info('rolled logfile to {}'.format(path))


def run_logger():
  kwargs = {
      'data_type_prefix': sys.argv[1],
  }
  if len(sys.argv) > 2:
    kwargs['max_output_size_bytes'] = int(sys.argv[2])
  if len(sys.argv) > 3:
    kwargs['max_file_duration_seconds'] = float(sys.argv[3])
  logger = DataLogger(**kwargs)

  for line in sys.stdin:
    logger.log_line(line)


if __name__ == '__main__':
  run_logger()
