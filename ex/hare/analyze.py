import sys
import numpy as np
import seaborn as sns
from collections import defaultdict, OrderedDict
from matplotlib import pyplot as plt
from pprint import pprint
from ujson import loads

from utils import zstd_open

def process(events):
  wss_events = defaultdict(dict)
  udp_events = defaultdict(dict)
  for evt in events:
    market = evt['s']
    if evt['source'] == 'wss':
      assert evt['t'] not in wss_events[market]
      wss_events[market][evt['t']] = evt
    elif evt['source'] == 'udp':
      assert evt['t'] not in udp_events[market]
      udp_events[market][evt['t']] = evt

  tkey = 'epochNanos'
  diffs = defaultdict(OrderedDict)
  for market, wss_evts in wss_events.items():
    for tid, wss_evt in wss_evts.items():
      if tid in udp_events[market]:
        diffs[market][tid] = (
            wss_evt[tkey] - udp_events[market][tid][tkey]) / 1e9
      else:
        diffs[market][tid] = None

  results = defaultdict(dict)
  for market, v in diffs.items():
    v_filtered = [(tid, vv) for tid, vv in v.items() if vv and abs(vv)]
    vs = [vv for _, vv in v_filtered]
    evts = wss_events[market]
    if 'USDT' in market:
      q_usd = [evts[tid]['p'] * evts[tid]['q'] for tid, vv in v_filtered]
    elif 'BTC' in market:
      q_usd = [6500 * evts[tid]['p'] * evts[tid]['q'] for tid, vv in v_filtered]
    g = sns.jointplot(q_usd, vs).set_axis_labels('quantity ($)', 'latency (s)')
    g.fig.suptitle(market)
    g.ax_joint.set_xscale('log')
    g.ax_marg_x.set_xscale('log')
    results[market]['mean'] = np.mean(vs)
    results[market]['median'] = np.median(vs)
    results[market]['min'] = np.min(vs)
    results[market]['max'] = np.max(vs)
    results[market]['std'] = np.std(vs)

  print('Positive indicates a time lead/advantage')
  pprint(results)
  plt.show()


def loads_or_skip(l):
  try:
    return loads(l)
  except ValueError:
    return None

def main(argv):
  events = []
  for path in sorted(argv):
    with zstd_open(path) as f:
      events.extend(e for e in (loads_or_skip(l) for l in f.readlines()) if e)
  process(events)

if __name__ == '__main__':
  main(sys.argv[1:])
