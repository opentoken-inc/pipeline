import sys
import os
import websocket
import time
import ujson as json
import _thread as thread
import logging
import inspect
from hashlib import sha1
from uuid import getnode
from datetime import datetime, timedelta
from itertools import chain

logger = logging.getLogger('scrape_ws')
logging.basicConfig(level=logging.DEBUG)

_MAC_HEX = hex(getnode())[2:]
_MAX_OUTPUT_SIZE = 10000000
_MAX_OUTPUT_DURATION = timedelta(seconds=180)


def mkdirs_exists_ok(path):
  try:
    os.makedirs(path)
  except OSError:
    if not os.path.isdir(path):
      raise


class CrashOnlyWebSocketApp(websocket.WebSocketApp):
  """WebSocketApp that does not catch exceptions from callbacks."""

  def _callback(self, callback, *args):
    if callback:
      if inspect.ismethod(callback):
        callback(*args)
      else:
        callback(self, *args)


class StreamConfig:

  def __init__(self, source, url):
    self.working_dir = 'working'
    self.output_dir = 'compressing'
    self.source = source
    self.url = url
    websocket.enableTrace(False)
    self.ws = CrashOnlyWebSocketApp(
        self.url,
        on_message=self.on_message,
        on_error=self.on_error,
        on_close=self.on_close)
    self.ws.on_open = self.on_open

    self.last_opened_time = None
    self.hasher = None
    self.f = None

    mkdirs_exists_ok(self.working_dir)
    mkdirs_exists_ok(self.output_dir)
    self._roll_logfile()

  def start(self):
    self.ws.run_forever(ping_timeout=1000)

  def on_open(self):
    pass

  def on_error(self, error):
    raise error

  def on_message(self, message):
    new_data = (message + '\n').encode()
    self.hasher.update(new_data)
    self.f.write(new_data)
    self._maybe_roll_logfile()

  def on_close(self):
    logger.error('websocket connection closed')
    self._close_logfile()

  def _maybe_roll_logfile(self):
    bytes_written = self.f.tell()
    now = datetime.utcnow()
    if (bytes_written > _MAX_OUTPUT_SIZE or
        now > self.last_opened_time + _MAX_OUTPUT_DURATION):
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
    path = '{}/ws_{}_{}_{}'.format(self.working_dir, self.source,
                                   now.strftime('%Y_%m_%d_%H_%M_%S'), _MAC_HEX)
    self.last_opened_time = now
    self.hasher = sha1()
    self.f = open(path, 'wb')
    logger.info('rolled logfile to {}'.format(path))


class BinanceStreamConfig(StreamConfig):

  def __init__(self):
    markets = (
        'btcusdt',
        'ethusdt',
        'ltcusdt',
        'ethbtc',
        'trxusdt',
    )
    streams = chain.from_iterable(('{}@trade'.format(market),
                                   '{}@depth'.format(market))
                                  for market in markets)

    super()('binance',
            'wss://stream.binance.com:9443/ws/stream?streams={}'.format(
                '/'.join(streams)))


class CoinbaseStreamConfig(StreamConfig):

  def __init__(self):
    super().__init__('coinbase', 'wss://ws-feed.pro.coinbase.com')

  def on_open(self):
    subscribe_msg = {
        'type': 'subscribe',
        'product_ids': [
            'ETH-USD',
            'BTC-USD',
            'LTC-USD',
            'ETH-BTC',
        ],
        'channels': ['heartbeat', 'level2', 'full']
    }
    self.ws.send(json.dumps(subscribe_msg))


def get_config_from_source(source):
  if source == 'binance':
    return BinanceStreamConfig()
  elif source == 'coinbase':
    return CoinbaseStreamConfig()
  else:
    raise NotImplementedError(source)


def main():
  source = sys.argv[1]
  config = get_config_from_source(source)
  logger.info('Running with source {}: {}'.format(source, config.url))

  config.start()


if __name__ == '__main__':
  main()
