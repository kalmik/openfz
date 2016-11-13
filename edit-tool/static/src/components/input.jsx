import React from 'react';
import ReactDOM from 'react-dom';

import ReactHighcharts from 'react-highcharts';

import { Glyphicon, Modal, Nav, NavItem, Button,
         Tabs, Tab, Row, Col, FormControl, ControlLabel,
         Panel } from 'react-bootstrap';

import fis from './fixture'

class BaseFis extends React.Component {

  constructor(...args) {
    super(...args);
    window.addEventListener('fis-change-state', (event) => {
      this.onChangeState.call(this, event)
    });
  }

  onChangeState(e) {
    console.log(this.state)
    this.setState(e.detail, () => {console.log(this.state)});
  }

  customSetState(state) {
    this.setState(state, () => {
      var event = new CustomEvent('fis-change-state', {detail: this.state});
      window.dispatchEvent(event);
    })
  }
}

class Functions extends React.Component {

  constructor(...args) {
    super(...args);
    var _series = this.buildSeries(this.props.data);

    var _state = {
      functions: this.props.data,
      series: _series,
      range: fis.inputs[0].range,
      inputTab: 0,
      inputChart: {},
      showCreate: false
    };

    this.state = _state;

    this.handleChange = this.handleChange.bind(this);
    this.handleInputTab = this.handleInputTab.bind(this);

    this.showCreate = this.showCreate.bind(this);
    this.close = this.close.bind(this);
    this.create = this.create.bind(this);
  }

  showCreate(e) {
    this.setState({showCreate: true});
  }

  close () {
    this.setState({showCreate: false});
  }

  create(e) {
    var name = ReactDOM.findDOMNode(this.refs.name).value;
    var data = {
      name: name,
      data:[0, 0, 0],
    };

    var currentState = this.state;
    currentState.functions.push(data);
    currentState.showCreate = false;
    this.setState(currentState);


  }

  buildSeries (funcs) {
    var _series = [];
    for (var i in funcs) {
      _series.push({
        name: funcs[i].name,
        data:[]
      });
      var last = funcs[i].data.length -1;
      for (var j in funcs[i].data) {
        var yValue = (j == last) || (j == 0) ? 0 : 1
        _series[i].data.push([+funcs[i].data[j], yValue]);
      }

    }

    return _series
  }

  getChartConfig (custom_state=false) {
    var state = this.state;
    if(custom_state) state = custom_state;

    var config =  {
      title: false,
      chart: {
        animation: false,
        type: 'area',
        height: 200
      },
      xAxis: {
        min: +state.range[0],
        max: +state.range[1]
      },
      yAxis: {
        min: 0,
        max: 1
      },
      series: this.buildSeries(state.functions),
      plotOptions: {
        series: {
          animation: false
        }
      },
    }
    return config
  }

  handleChange (event, i, j) {
    var currentState = this.state;

    currentState.functions[i].data[j] = +event.target.value;
    this.setState(currentState);
  }

  handleInputTab (key) {
    if (key=="add") {
      this.setState({showCreate: true});
      return;
    }
    this.setState({inputTab: key});
  }

  changeType (event, i, where) {
    var value = event.target.value;
    var currentState = this.state;

    if (value == 4) {
      currentState.functions[i].data.push(0);
    } else {
      currentState.functions[i].data.pop();
    }
    this.setState(currentState);
  }

  render() {
    var _this = this;
    return (
      <section>
        <ReactHighcharts config={this.getChartConfig()}></ReactHighcharts>
        <Tabs defaultActiveKey={0} activeKey={this.state.inputTab} onSelect={this.handleInputTab} id="input_tabs">
        {
          this.state.functions.map( (el, i) => {
            return (
                <Tab eventKey={i} key={"input_"+i} title={el.name}>
                  <Row style={{marginTop:10}}>
                      <Col md={12} >
                        <ControlLabel>Type</ControlLabel>
                        <FormControl componentClass="select" placeholder="select" value={el.data.length}
                                     onChange={(event) => {this.changeType(event, i, 'input')}}>
                          <option value="3">Trimf</option>
                          <option value="4">Trapmf</option>
                        </FormControl>
                      </Col>
                      <Col md={12} style={{marginBottom: 10}}>
                        {
                          el.data.map( (el2, j) => {
                            return(
                              <Col xs={3} style={{marginTop: 25}}>
                                <FormControl
                                  type="number"
                                  key={"input_"+i+"_"+j}
                                  defaultValue={el2}
                                  placeholder="Enter text"
                                  step="0.1"
                                  onChange={(event) =>{this.handleChange(event, i, j)}}/>
                              </Col>
                            )
                          })
                        }
                      </Col>
                      <Col md={12}>
                        <ControlLabel>Range</ControlLabel>
                        <FormControl
                          type="text"
                          defaultValue={this.state.range.join(', ')}
                          placeholder="Enter text"
                          onChange={_this.handleChange} />
                      </Col>
                  </Row>
                </Tab>
              )
          })
        }
          <Tab eventKey={"add"} title={<Glyphicon glyph="plus" />}></Tab>
        </Tabs>
        <Modal
          show={this.state.showCreate}
          onHide={this.close}
          container={this}
          aria-labelledby="contained-modal-title"
        >
          <Modal.Header closeButton>
            <Modal.Title id="contained-modal-title">Create new</Modal.Title>
          </Modal.Header>
          <Modal.Body>
            <ControlLabel>Name</ControlLabel>
            <FormControl
              type="text"
              ref="name"
              placeholder="Enter text"/>
          </Modal.Body>
          <Modal.Footer>
            <Button onClick={this.create}>Create</Button>
          </Modal.Footer>
        </Modal>
      </section>
    )
  }
}


class InputPanel extends BaseFis {
  constructor(...args) {
    super(...args);

    this.state = {
      input: this.props.data,
      showCreate: false
    };

    this.showCreate = this.showCreate.bind(this);
    this.close = this.close.bind(this);
    this.create = this.create.bind(this);

  }

  showCreate(e) {
    this.setState({showCreate: true});
  }

  close () {
    this.setState({showCreate: false});
  }

  create(e) {
    var name = ReactDOM.findDOMNode(this.refs.name).value;
    var data = {
      name: name,
      functions:[],
      range: [-1, 1]
    };

    var currentState = this.state;
    currentState.input.push(data);
    currentState.showCreate = false;
    this.customSetState(currentState);

  }

  render () {
    return (
      <Tab.Container id="left-tabs-example" defaultActiveKey={this.state.input[0].name}>
        <Row className="clearfix">
          <Col xs={2}>
            <Nav bsStyle="pills" stacked>
            {
              this.state.input.map( (el, i) => {
                return (
                    <NavItem eventKey={el.name}>
                      {el.name}
                    </NavItem>
                )
              })
            }

            </Nav>
            <Button bsStyle="success" onClick={this.showCreate} style={{margin: 5}}>
              <Glyphicon glyph="plus" />
            </Button>
          </Col>
          <Col xs={10}>
            <Tab.Content animation>
              {
                this.state.input.map( (el, i) => {
                  return (
                    <Tab.Pane eventKey={el.name}>
                      <Functions data={el.functions}/>
                    </Tab.Pane>
                  )
                })
              }

            </Tab.Content>
            <Modal
              show={this.state.showCreate}
              onHide={this.close}
              container={this}
              aria-labelledby="contained-modal-title"
            >
              <Modal.Header closeButton>
                <Modal.Title id="contained-modal-title">Create new</Modal.Title>
              </Modal.Header>
              <Modal.Body>
                <ControlLabel>Name</ControlLabel>
                <FormControl
                  type="text"
                  ref="name"
                  placeholder="Enter text"/>
              </Modal.Body>
              <Modal.Footer>
                <Button onClick={this.create}>Create</Button>
              </Modal.Footer>
            </Modal>
          </Col>
        </Row>
      </Tab.Container>
    )
  }
}

class OutputPanel extends InputPanel {}

class Rules extends BaseFis {

  constructor (...args) {
    super(...args);

    this.state = this.props.data
  }

  serialize () {
    return "oi"
  }

  render () {
    return (
      <Row>
       <Col xs={3}></Col>
      </Row>
    )
  }
}

class App extends React.Component {

  constructor (...args) {
    super(...args);

    this.state = {fis: this.props.data}

    this.serialize = this.serialize.bind(this);

  }

  serialize () {
    console.log(this);
  }

  render () {
    return (
      <section>
        <Row>
          <Col md={6}>
            <Panel header="Inputs">
              <InputPanel data={this.state.fis.inputs} ref="input"/>
            </Panel>
          </Col>

          <Col md={6}>
            <Panel header="Outputs">
              <OutputPanel data={this.state.fis.outputs} ref="output"/>
            </Panel>
          </Col>

        </Row>

        <Row class="row">
          <Col md={12}>
            <Panel header="Rules">
              <Rules ref="rules"/>
            </Panel>
          </Col>
        </Row>
      </section>
    )
  }
}

ReactDOM.render(<App data={fis}/>, document.querySelector("#app"));
