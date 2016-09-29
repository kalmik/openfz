import React from 'react';
import ReactDOM from 'react-dom';
import ReactHighcharts from 'react-highcharts';

import fis from './fixture' 
var Input = React.createClass ({

  getInitialState (){
    var _series = []

    var funcs = fis.inputs[0].functions;
    for (var i in funcs) {
      _series.push({
        name: funcs[i].name,
        data:[]
      });

      var last = funcs[i].data.length -1;
      for (var j in funcs[i].data) {
        var yValue = (j == last) || (j == 0) ? 0 : 1
        _series[i].data.push([funcs[i].data[j], yValue]);
      }

    }

    var _state = {
      functions: fis.inputs[0].functions,
      series: _series,
      range: fis.inputs[0].range
    };

    return _state;
  },

  getChartConfig () {
    return {
      chart: {
        type: 'area',
        height: 200
      },
      xAxis: {
        min: +this.state.range[0],
        max: +this.state.range[1]  
      },
      yAxis: {
        min: 0,
        max: 1
      },
      series: this.state.series
    }
  },

  render() {
    console.log(this.state)

    return <ReactHighcharts config={this.getChartConfig()}></ReactHighcharts>
  }
})
ReactDOM.render(<Input/>, document.querySelector('#input-pane .panel-body'));
