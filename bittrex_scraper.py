import time
import sys
import logging
import requests
from json import dumps
from ujson import loads
from base64 import b64decode
from zlib import decompress

from data_logger import DataLogger
from signalr_custom import SynchronousSignalrConnection

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)


_QUERY_PERIOD = 240.


def crash_and_exit(error):
  print(error)
  sys.exit(1)


def decompress_arg(a):
  decoded = b64decode(a)

  # Use negative -15 to say no header and max size.
  # The window can be bigger than the one used during compression, but not smaller.
  decompressed = decompress(decoded, -15)
  dat_obj = loads(decompressed)
  return dat_obj


def decompress_message(m):
  result = m.copy()
  if 'A' in m:
    result['A'] = [decompress_arg(a) for a in m['A']]
  return result


class BittrexStreamConfig(DataLogger):

  def __init__(self):
    super().__init__(
        'ws_{}'.format('bittrex'),
        max_output_size_bytes=25000000,
        max_file_duration_seconds=60 * 10)

    self.url = 'https://socket.bittrex.com/signalr'
    self.received_counter = 0
    self.last_received_time = None
    self.last_query_time = None
    self.invocation_ids = {}


    markets = (
        'USDT-BTC',
        'USDT-ETH',
        'BTC-ETH',
        'BTC-XLM',
        'BTC-LTC',
        'BTC-XRP',
        'BTC-BCH',
        'BTC-ADA',
    )
    self.invocations_args = (
        [('QuerySummaryState',), ('SubscribeToSummaryDeltas',),
         ('SubscribeToSummaryDeltas',)] + [
             ('QueryExchangeState', market) for market in markets
         ] + [('SubscribeToExchangeDeltas', market) for market in markets])

  def start(self):
    with requests.Session() as session:
      return self._start_with_session(session)

  def _start_with_session(self, session):
    connection = SynchronousSignalrConnection(self.url, session)
    connection.on_open = self._on_open
    self.hub = connection.register_hub('c2')
    connection.received += self._on_receive
    connection.error += crash_and_exit
    try:
      connection.start()
    finally:
      connection.close()

  def _on_receive(self, **kwargs):
    now = time.time()
    self.last_received_time = now
    self.received_counter += 1
    if self.received_counter % 1000 == 0:
      logger.info('received {} messages'.format(self.received_counter))

    if not kwargs:
      return

    msg = kwargs.copy()

    invocation_args = self.invocation_ids.pop(msg.get('I'), None)
    if invocation_args:
      msg['responseTo'] = invocation_args

    if 'M' in msg:
      msg['M'] = [decompress_message(m) for m in msg['M']]
    if 'R' in msg and isinstance(msg['R'], str):
      msg['R'] = decompress_arg(msg['R'])

    self.log_json(msg)
    self._maybe_redo_queries()


  def _on_open(self):
    self._maybe_redo_queries()


  def _maybe_redo_queries(self):
    lq = self.last_query_time
    now = time.time()
    if not (lq is None or now > lq + _QUERY_PERIOD):
      return

    logger.info('querying order books')
    self.last_query_time = now
    for invocation_args in self.invocations_args:
      invocation_id = self.hub.server.invoke(*invocation_args)
      self.invocation_ids[str(invocation_id)] = invocation_args
