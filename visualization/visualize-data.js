const PLOT = d3.select('div#plot')
  .append('svg')
  .classed('svg-container', true)
  .classed('svg-content-responsive', true);
const SLIDER = document.getElementById('slider-input');
const TOOLTIP = d3.select('div#plot').append('div')
  .attr('class', 'tooltip');

var STATE = {
  rawData: null,
  state: null,
  updating: false
};

/* Process data */

var cumSum = function(orderList) {
  var total = 0;
  var results = [];
  orderList.forEach(function(o) {
    total += o.q;
    results.push({ p: o.p, q: total, t: o.t });
  });
  return results;
};

var preprocessState = function(rawState) {
  var bidsMap = rawState.bids;
  var asksMap = rawState.asks;

  var bidsList = Object.keys(bidsMap).map(function(k) {
    return { 'p': Number(k), 'q': Number(bidsMap[k]), 't': 'BID' };
  }).sort(function(a, b) { return b.p - a.p; });
  var asksList = Object.keys(asksMap).map(function(k) {
    return { 'p': Number(k), 'q': Number(asksMap[k]), 't': 'ASK' };
  }).sort(function(a, b) { return a.p - b.p; });

  /* Test */
  cumSum(bidsList).forEach(function(d, i) {
    if (i > 0 && d.q < bidsList[i-1].q) {
      console.error('Error in cumSum (bids)', i, d, bidsList[i-1]);
    }
  });
  cumSum(asksList).forEach(function(d, i) {
    if (i > 0 && d.q < asksList[i-1].q) {
      console.error('Error in cumSum (asks)', i, d, asksList[i-1]);
    }
  });

  return cumSum(bidsList).reverse().concat(cumSum(asksList));
};

var getDeltaList = function(startNonce, endNonce) {
  const deltas = STATE.rawData.deltas;
  // Forward
  if (startNonce <= endNonce) {
    const startIdx = deltas.findIndex(function(d) { return d.nonce === startNonce });
    const endIdx = deltas.findIndex(function(d) { return d.nonce === endNonce+1 });
    return deltas.slice(startIdx, endIdx);
  }
  // Reverse
  else {
    const startIdx = deltas.findIndex(function(d) { return d.nonce === endNonce });
    const endIdx = deltas.findIndex(function(d) { return d.nonce === startNonce });
    return deltas.slice(startIdx, endIdx).reverse();
  }
};

var applyDeltaToState = function(state, delta, reverse) {
  var key = delta.type === 'BID' ? 'bids' : 'asks';
  var bidOrAskState = state[key];

  if (reverse) {
    switch(delta.TY) {
      case 0:
        // NOTE: Apparently delete is slow. Consider setting to null
        //       https://jsperf.com/delete-vs-undefined-vs-null/16
        delete bidOrAskState[delta.R];
        break;
      case 1:
      case 2:
      default:
        bidOrAskState[delta.R] = delta.Q;
        break;
    }
  }
  else {
    switch(delta.TY) {
      case 1:
        // NOTE: Apparently delete is slow. Consider setting to null
        //       https://jsperf.com/delete-vs-undefined-vs-null/16
        delete bidOrAskState[delta.R];
        break;
      case 0:
      case 2:
      default:
        bidOrAskState[delta.R] = delta.Q;
        break;
    }
  }
};

var draw = function(state, plot) {
  /* Plot helpers */
  var onMouseOver = function(d) {
    //TOOLTIP.transition().duration(500).style('opacity', 1);
    var html = `<p>Price: ${d.p}<br/>Quantity: ${d.q}<br/>Nonce: ${state.nonce}</p>`;
    TOOLTIP.html(html);
  };

  var onMouseOut = function(d) {
    //TOOLTIP.transition().duration(500).style('opacity', 0);
  };

  var orderBook = preprocessState(state);

  var margin = { top: 20, right: 20, bottom: 20, left: 20 };
  var WIDTH = plot.node().clientWidth - margin.left - margin.right;
  var HEIGHT = plot.node().clientHeight - margin.top - margin.bottom;
  var x = d3.scaleLinear().range([0, WIDTH]);
  x.domain([d3.min(orderBook, function(o) { return o.p }),
            d3.max(orderBook, function(o) { return o.p })]);
  var y = d3.scaleLinear().range([HEIGHT, 0]);
  y.domain([0, d3.max(orderBook, function(o) { return o.q })]);

  var selection = plot.selectAll('bar')
    .data(orderBook, function(d) { return d.p; }) // Specify a key to match for
                                                  // updates
  selection.enter()
    .append('rect')
    .attr('class', function(d) { return 'bar ' + d.t.toLowerCase(); })
    .attr('x', function(d) { return x(d.p); })
    .attr('y', function(d) { return y(d.q); })
    .attr('width', function(d, i) {
      if (orderBook[i+1] && d.t === orderBook[i+1].t) {
        return x(orderBook[i+1].p) - x(d.p) + 1;
      }
    })
    .attr('height', function(d) { return HEIGHT -  y(d.q); })
    .on('mouseover', onMouseOver)
    .on('mouseout', onMouseOut);
  selection.exit().remove();
};

/* Bind slider events */

SLIDER.oninput = function() {
  if (STATE.updating) { console.log('Already updating'); return; }
  STATE.updating = true;

  var currentNonce = STATE.state.nonce;
  var nextNonce = Number(this.value);
  var reverse = (nextNonce < currentNonce);
  var deltaList = getDeltaList(currentNonce, nextNonce);

  deltaList.forEach(function(delta) {
    applyDeltaToState(STATE.state, delta, reverse);
    STATE.state.nonce = delta.nonce;
  });
  console.log('Redrawing state');
  draw(STATE.state, PLOT);

  STATE.updating = false;
};

/* Load data */

console.log('Fetching');
d3.json('bittrex-dump.json')
  .then(function(data) {
    console.log('Data', data);
    var initialState = data.states[0];
    var lastDelta = data.deltas[data.deltas.length-1];
    STATE = {
      rawData: data,
      state: initialState
    };
    SLIDER.setAttribute('value', initialState.nonce);
    SLIDER.setAttribute('min', initialState.nonce);
    SLIDER.setAttribute('max', lastDelta.nonce);
    draw(initialState, PLOT);
  })
  .catch(function(err) {
    console.log('Error', err);
  });

