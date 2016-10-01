import React from 'react';
import ReactDOM from 'react-dom';

import ReactHighcharts from 'react-highcharts';

import { Button, Tabs, Tab, Row, Col, FormControl, ControlLabel } from 'react-bootstrap';

import fis from './fixture'
class Input extends React.Component {

  constructor(...args) {
    super(...args);
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

    this.state = _state;
  }

  getChartConfig () {
    return {
      chart: {
        animation: false,
        title: false,
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
      series: this.state.series,
      plotOptions: {
        series: {
          animation: false
        }
      },
    }
  }

  render() {
    console.log(this.state)

    return (
      <section>
        <ReactHighcharts config={this.getChartConfig()}></ReactHighcharts>
        <Tabs defaultActiveKey={0} id="uncontrolled-tab-example">
        {
          this.state.functions.map( (el, i) => {
            return (
                <Tab eventKey={i} key={"input"+i} title={el.name}>
                  <Row style={{marginTop:10}}>
                      <Col md={4} >
                        <ControlLabel>Type</ControlLabel>
                        <FormControl componentClass="select" placeholder="select">
                          <option value="select">Trimf</option>
                          <option value="other">Trapmf</option>
                        </FormControl>
                      </Col>
                      <Col md={4}>
                        <ControlLabel>Value</ControlLabel>
                        <FormControl
                          type="text"
                          value={el.data.join(', ')}
                          placeholder="Enter text"
                          onChange={this.handleChange} />
                      </Col>
                      <Col md={4}>
                        <ControlLabel>Range</ControlLabel>
                        <FormControl
                          type="text"
                          value={this.state.range.join(', ')}
                          placeholder="Enter text"
                          onChange={this.handleChange} />
                      </Col>
                  </Row>
                </Tab>
              )
          })
        }
        </Tabs>
      </section>
    )
  }
}

ReactDOM.render(<Input/>, document.querySelector('#input-pane .panel-body'));
