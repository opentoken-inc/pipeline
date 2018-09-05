import sys
from collections import defaultdict
from ujson import loads

from utils import zstd_open, pretty_diff_changes


def on_snapshot(order_books, msg):
  market = msg['responseTo'][1]
  nonce = msg['R']['N']
  if nonce < order_books['nonces'][market]:
    print('snapshot skipped for {}'.format(market))
    return

  snapshot = {
      k: {obj['R']: obj['Q']
          for obj in msg['R'][k]}
      for k in ('Z', 'S')
  }

  if market in order_books:
    if order_books[market] != snapshot:
      print(pretty_diff_changes(order_books[market], snapshot))
      raise AssertionError('mismatch snapshot for {}'.format(market))
    else:
      print('snapshot matches for {}'.format(market))
  else:
    order_books[market] = snapshot

  order_books['nonces'][market] = nonce


def on_delta(order_books, msg):
  for M in msg['M']:
    if M['M'] != 'uE':
      continue
    delta = M['A'][0]
    market = delta['M']
    if market not in order_books:
      return
    nonce = delta['N']
    if nonce < order_books['nonces'][market]:
      return

    for ot in ('Z', 'S'):
      for dd in delta[ot]:
        if dd['Q'] == 0:
          order_books[market][ot].pop(dd['R'], None)
        else:
          order_books[market][ot][dd['R']] = dd['Q']

    order_books['nonces'][market] = nonce


def is_snapshot(msg):
  return msg.get('responseTo', [None])[0] == 'QueryExchangeState'


def is_delta(msg):
  return 'M' in msg and any(M.get('M') == 'uE' for M in msg['M'])


def process_lines(order_books, lines):
  for msg in map(loads, lines):
    if is_delta(msg):
      on_delta(order_books, msg)
    elif is_snapshot(msg):
      on_snapshot(order_books, msg)


def main(argv):
  paths = sorted(argv)
  order_books = {'nonces': defaultdict(lambda: -1)}
  for path in paths:
    with zstd_open(path) as f:
      lines = (l for l in f.read().split('\n') if l)
    process_lines(order_books, lines)


if __name__ == '__main__':
  main(sys.argv[1:])
