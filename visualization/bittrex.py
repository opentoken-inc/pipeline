"""
Notes:
* Filter out the following responses: SubscribeToSummaryDeltas,
                                      SubscribeToExchangeDeltas

"""


import datetime
import json
import re
import os


def process_ws_json(data):
    # Exchange state
    if 'responseTo' in data:
        if data['responseTo'][0] == 'QueryExchangeState':
            market = data['responseTo'][1]
            state = {}
            state[market] = {
                'bids': { d['R']: d['Q'] for d in data['R']['Z'] },
                'asks': { d['R']: d['Q'] for d in data['R']['S'] },
                'nonce': data['R']['N'],
                'logTime': data['logTime']
            }
            return ('STATE', state)
        # Ignore SubscribeToExchangeDeltas, SubscribeToSummaryDeltas
        # TODO: Handle QuerySummaryState
        else:
            return (None, None)
    # Deltas
    elif 'C' in data:
        deltas = {}
        for mktdata in data['M']:
            # Market delta
            if mktdata['M'] == 'uE':
                for mktdelta in mktdata['A']:
                    deltas.setdefault(mktdelta['M'], []).extend(
                        [{'type': 'BID', 'nonce': mktdelta['N'], **bid}
                         for bid in mktdelta['Z']]
                    )
                    deltas.setdefault(mktdelta['M'], []).extend(
                        [{'type': 'ASK', 'nonce': mktdelta['N'], **ask}
                         for ask in mktdelta['S']]
                    )
            # TODO: Order delta (uO) and summary delta (uS)
        for mkt, d in deltas.items():
            assert(len(set([x['nonce'] for x in d])) <= 1)  # Up to 1 nonce per market

        return ('DELTA', deltas)
    # Unhandled responses
    else:
        return (None, None)


def update_order_book_state(prev_state, delta):
    """Mutates @prev_state"""
    if delta['nonce'] <= prev_state['nonce']:
        return

    key = 'bids' if delta['type'] == 'BID' else 'asks'

    if delta['TY'] == 0:
        prev_state[key][delta['R']] = delta['Q']
        print('Added order to book', delta)
    elif delta['TY'] == 1 or delta['TY'] == 2:
        assert(delta['R'] in prev_state[key])
        if delta['TY'] == 1:
            prev_state[key].pop(delta['R'])  # Remove
            print('Removed order from book', delta)
        else:
            prev_state[key][delta['R']] = delta['Q']  # Update
            print('Updated order to', delta)


def iterate_order_book(initial_state, deltas):
    state = initial_state
    for next_delta in deltas:
        yield(state, next_delta)
        update_order_book_state(state, next_delta)
    yield (state, None)


def convert_order_book_dict_to_list(book):
    """@book: Dict of bids or asks {[rate]: [quantity]}"""
    return [(k, book[k]) for k in sorted(book.keys())]


class BittrexWrapper:

    __DATA_DIR = 'data/bittrex'
    __FILENAME_DATETIME_REGEX = (r'[0-9]{4}_[0-9]{2}_[0-9]{2}_'
                                  '[0-9]{2}_[0-9]{2}_[0-9]{2}')


    def __init__(self, start_date=None, end_date=None):
        """
        @self.book_states = {
            [market name]: [{
                'bids': {[rate]: [quantity]},  # This is a map for faster delta
                                               # updates
                'asks': {[rate]: [quantity]},
                'nonce': [nonce],
                'logTime': [timestamp],
            }]
        }
        @self.book_deltas = {
            [market name]: [{
                'type': ['bid' or 'ask'],
                'R': [rate],
                'Q': [quantity],
                'TY': [create/update/delete]
            }]
        }
        """
        # TODO: Change delta format
        self.book_states = {}
        self.book_deltas = {}
        self.raw_data = self._load_raw_data(start_date, end_date)

        for d in self.raw_data:
            t, results = process_ws_json(d)
            if t == 'STATE':
                for market, state in results.items():
                    self.book_states.setdefault(market, []).append(state)
            elif t == 'DELTA':
                for market, deltas in results.items():
                    # TODO: This doesn't ensure order if elements in raw_data
                    #       are out of order
                    (self.book_deltas.setdefault(market, [])
                                     .extend(sorted(deltas, key=lambda x: x['nonce'])))


    def __extract_datetime_from_filename(self, filename):
        match = re.search(self.__FILENAME_DATETIME_REGEX, filename)
        if match:
            date_list = match.group().split('_')
            return datetime.datetime(*map(int, date_list))


    def _load_raw_data(self, start_date, end_date):
        data = []
        for filename in os.listdir(self.__DATA_DIR):
            if filename[-5:] == '.json':
                file_datetime = self.__extract_datetime_from_filename(filename)
                if ((start_date and start_date > file_datetime)
                    or (end_date and end_date < file_datetime)):
                    continue
                else:
                    with open(os.path.join(self.__DATA_DIR, filename), 'r') as f:
                        for line in f:
                            data.append(json.loads(line))
        return data


    def get_markets(self):
        return set(list(book_states.keys()))


    def get_earliest_order_book_snapshot(self, market, start_date=None,
                                         end_date=None):
        for state in self.book_states[market]:
            if ((start_date is None
                 or datetime.datetime.fromtimestamp(state['logTime']) > start_date)
                and (end_date is None
                     or datetime.datetime.fromtimestamp(state['logTime']) < end_date)):
                return state


    def replay_order_book_state(self, market, start_date=None, end_date=None):
        book_state = self.get_earliest_order_book_snapshot(market, start_date, end_date)
        for prev_state, next_delta in iterate_order_book(book_state, self.book_deltas[market]):
            print('Nonce', next_delta['nonce'])
            print('Highest bid', sorted(prev_state['bids'].keys())[-1])
            print('Lowest ask', sorted(prev_state['asks'].keys())[0])
            print('Sum bids', sum(prev_state['bids'].values()))
            print('Sum asks', sum(prev_state['asks'].values()))


    def replay_order_book_state_to_nonce(self, market, nonce):
        book_state = self.get_earliest_order_book_snapshot(market)
        print(book_state, 'Book state')
        for prev_state, next_delta in iterate_order_book(book_state, self.book_deltas[market]):
            print(next_delta['nonce'], nonce)
            if next_delta['nonce'] > nonce:
                return prev_state
        return


    def dump_state_and_deltas(self, market, filename):
        with open(filename, 'w') as f:
            json.dump({
                'states': self.book_states[market],
                'deltas': self.book_deltas[market]
            }, f)


def compare_order_books():
    b = BittrexWrapper()
    last_book_state = b.book_states['BTC-ETH'][-1]
    replayed_book_state = b.replay_order_book_state_to_nonce('BTC-ETH',
                                                             last_book_state['nonce'])
    # Compare bids
    last_bids = convert_order_book_dict_to_list(last_book_state['bids'])
    replayed_bids = convert_order_book_dict_to_list(replayed_book_state['bids'])
    print('Comparing bids')
    for i, _ in enumerate(last_bids):
        print(last_bids[i], replayed_bids[i])
        assert(last_bids[i] == replayed_bids[i])
    # Compare asks
    last_asks = convert_order_book_dict_to_list(last_book_state['asks'])
    replayed_asks = convert_order_book_dict_to_list(replayed_book_state['asks'])
    print('Comparing asks')
    for i, _ in enumerate(last_asks):
        print(last_asks[i], replayed_asks[i])
        assert(last_asks[i] == replayed_asks[i])

    return (last_book_state, replayed_book_state)

