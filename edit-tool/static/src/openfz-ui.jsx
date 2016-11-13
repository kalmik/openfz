import Ajax from './ajax.jsx'
import React from 'react';
import ReactDOM from 'react-dom';


const _ajax = new Ajax;


class OpenfzUI extends React.Component {
  constructor(...args) {
    super(...args);

    _ajax.get('/list', (response) => {
      console.log(response);
    });;
  }

  render () {
    return (
      <div>
        Bla
      </div>
    )
  }
}


ReactDOM.render(<OpenfzUI />, document.querySelector("#app"));