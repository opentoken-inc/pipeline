import sys
import os
import requests
import logging
import time
import ujson as json
from itertools import chain

from data_logger import DataLogger

logger = logging.getLogger('scrape_ws')
logging.basicConfig(level=logging.DEBUG)


class RestConfig(DataLogger):

  def __init__(self, source, schedule):
    super().__init__('rest_{}'.format(source))


class CoinbaseRestConfig(DataLogger):

  def __init__(self):
    super().__init__('coinbase')


def get_config_from_source(source):
  if source == 'binance':
    return BinanceRestConfig()
  elif source == 'coinbase':
    return CoinbaseRestConfig()
  else:
    raise NotImplementedError(source)


def main():
  source = sys.argv[1]
  config = get_config_from_source(source)
  logger.info('Running with source {}: {}'.format(source, config.url))

  config.start()


if __name__ == '__main__':
  main()
