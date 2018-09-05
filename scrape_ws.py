import sys
import os
import websocket
import logging
import inspect
import requests
import time
from gzip import decompress as gzip_decompress
from json import dumps
from itertools import chain

from data_logger import DataLogger

logger = logging.getLogger('scrape_ws')
logging.basicConfig(level=logging.INFO)


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


class StreamConfig(DataLogger):

  def __init__(self, source, url, **kwargs):
    super().__init__('ws_{}'.format(source), **kwargs)
    self.url = url
    websocket.enableTrace(False)
    self.ws = CrashOnlyWebSocketApp(
        self.url,
        on_message=self.on_message,
        on_error=self.on_error,
        on_close=self.on_close)
    self.ws.on_open = self.on_open

  def start(self):
    self.ws.run_forever(ping_timeout=1000)

  def on_open(self):
    pass

  def on_error(self, error):
    raise error

  def on_message(self, message):
    self.log_json(message)

  def on_close(self):
    logger.error('websocket connection closed')
    self._close_logfile()


class BinanceStreamConfig(StreamConfig):
  _QUERY_PERIOD = 360.

  def __init__(self):
    self.markets = (
        'btcusdt',
        'ethusdt',
        'ltcusdt',
        'ethbtc',
        'trxusdt',
        'zrxbtc',
    )
    streams = (['!ticker@arr'] + list(
        chain.from_iterable(
            ('{}@trade'.format(market), '{}@depth'.format(market))
            for market in self.markets)))

    super().__init__(
        'binance',
        'wss://stream.binance.com:9443/ws/{}'.format('/'.join(streams)),
        max_output_size_bytes=13000000,
        max_file_duration_seconds=10 * 60)


    self.last_query_time = None

  def on_open(self):
    self._maybe_redo_queries()

  def on_message(self, message):
    self.log_json(message)
    self._maybe_redo_queries()

  def _maybe_redo_queries(self):
    lq = self.last_query_time
    now = time.time()
    if not (lq is None or now > lq + self._QUERY_PERIOD):
      return

    logger.info('querying order books')
    self.last_query_time = now
    for market in self.markets:
      result = requests.get('https://www.binance.com/api/v1/depth?symbol={}&limit=1000'.format(market.upper()))
      result.raise_for_status()
      book_json = result.json()

      assert 'e' not in book_json
      assert 's' not in book_json
      book_json['e'] = 'depth'
      book_json['s'] = market
      self.log_json(book_json)
      time.sleep(1.)


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
    for product_id in subscribe_msg['product_ids']:
      result = requests.get('https://api.pro.coinbase.com/products/{}/book?level=3'.format(product_id))
      result.raise_for_status()
      book_json = result.json()
      book_json['product_id'] = product_id
      self.log_json(book_json)

    self.ws.send(dumps(subscribe_msg))


class BitmexStreamConfig(StreamConfig):

  def __init__(self):
    super().__init__('bitmex', 'wss://www.bitmex.com/realtime')

  def on_open(self):
    markets = ('XBTUSD',)
    subscriptions = ['subscribe:{}'.format(market) for market in markets]
    for subscription in subscription:
      raise NotImplementedError()


class HuobiStreamConfig(StreamConfig):

  def __init__(self):
    super().__init__(
        'huobi', 'wss://api.huobi.pro/ws',
        max_file_duration_seconds=200)
    self.last_pong_time = time.time()

  def on_message(self, message):
    result = gzip_decompress(message)
    self.log_json(result)
    self._maybe_pong()

  def on_open(self):
    markets = ('btcusdt', 'ethusdt', 'ethbtc', 'eosusdt', 'bchusdt', 'xrpusdt',
               'etcusdt')
    subs_fmts = (
        'market.{}.detail',
        'market.{}.kline.1day',
        'market.{}.depth.percent10',
        'market.{}.trade.detail',
        'market.{}.depth.step0',
    )

    subs = ['market.overview'] + [sf.format(market) for market in markets for sf in subs_fmts]

    for sub in subs:
      self.ws.send(dumps({'sub': sub}))

  def _maybe_pong(self):
    now = time.time()
    if now - self.last_pong_time > 4.8:
      self.ws.send(dumps({'pong': 1000*int(now)}))
      self.last_pong_time = now


def get_config_from_source(source):
  if source == 'binance':
    return BinanceStreamConfig()
  elif source == 'coinbase':
    return CoinbaseStreamConfig()
  elif source == 'huobi':
    return HuobiStreamConfig()
  elif source == 'bitmex':
    return BitmexStreamConfig()
  elif source == 'bittrex':
    from bittrex_scraper import BittrexStreamConfig
    return BittrexStreamConfig()
  else:
    raise NotImplementedError(source)


def main():
  source = sys.argv[1]
  config = get_config_from_source(source)
  logger.info('Running with source {}: {}'.format(source, config.url))

  config.start()


if __name__ == '__main__':
  main()
