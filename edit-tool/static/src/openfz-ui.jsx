import Ajax from './ajax.jsx'
import React from 'react';
import ReactDOM from 'react-dom';
import MuiThemeProvider from 'material-ui/styles/MuiThemeProvider';
import {Card, CardActions, CardHeader, CardText} from 'material-ui/Card';
import FlatButton from 'material-ui/FlatButton';

import {List, ListItem} from 'material-ui/List';
import ContentInbox from 'material-ui/svg-icons/content/inbox';
import ActionGrade from 'material-ui/svg-icons/action/grade';
import ContentSend from 'material-ui/svg-icons/content/send';
import ContentDrafts from 'material-ui/svg-icons/content/drafts';
import Divider from 'material-ui/Divider';
import ActionInfo from 'material-ui/svg-icons/action/info';
import AlertWarning from 'material-ui/svg-icons/alert/warning';
import Paper from 'material-ui/Paper';
import Chip from 'material-ui/Chip';
import AppBar from 'material-ui/AppBar';
import IconButton from 'material-ui/IconButton';
import Subheader from 'material-ui/Subheader'
const _ajax = new Ajax;

import AddButton from 'material-ui/svg-icons/content/add'
import RemoveButton from 'material-ui/svg-icons/content/remove'

function handleTouchTap() {
  alert('onTouchTap triggered on the title component');
}

const styles = {
  title: {
    cursor: 'pointer',
  },
};

class OpenfzUI extends React.Component {
  constructor(...args) {
    super(...args);

    this.state = {
      online: [],
      offline: []
    };

    this.reload();
  }

  reload() {
    console.log("Reloading")
    _ajax.get('/summary', (response) => {
      this.setState({
        online: JSON.parse(response)
      });
    });

    _ajax.get('/list', (response) => {
      this.setState({
        offline: JSON.parse(response)
      });
    });

  }

  load (name) {
    _ajax.put(`/load/${name}`, {}, (response) => {
      if(response) this.reload()
    });
  }

  unload (slot) {
    _ajax.put(`/unload/${slot}`, {}, (response) => {
      if(response) this.reload()
    });
  }

  render () {
    return (
      <div>
        <AppBar
          title={<span style={styles.title}>OpenFz UI</span>}
          onTitleTouchTap={handleTouchTap}
          iconElementRight={<FlatButton label="New" />}
        />
        <Paper zDepth={2} >
          <List>
            <Subheader>MFIS Online</Subheader>
          {
            this.state.online.map((el, i) => {
              let color = el.client ? '#9C0' : '#CCC';
              let client_ip = el.client ? <Chip>{el.client}</Chip> : <RemoveButton onClick={() => {
                this.unload(el.slot);
              }} />;
              return (
                <div>
                  <ListItem key={`online_${i}`} primaryText={el.name} rightAvatar={client_ip}
                            leftIcon={<ActionInfo color={color} />} />
                  <Divider />
                </div>
              )
            })
          }
          </List>
          <List>
            <Subheader>MFIS Offline</Subheader>
            {
              this.state.offline.map((el, i) => {
                return (
                  <div>
                    <ListItem key={`offline_${i}`} primaryText={el} rightIcon={<AddButton onClick={() => {
                      this.load(el);
                    }} />} />
                    <Divider />
                  </div>
                )
              })
            }
          </List>
        </Paper>
      </div>
    )
  }
}

const App = () => (
  <MuiThemeProvider>
    <OpenfzUI />
  </MuiThemeProvider>
);

ReactDOM.render(
  <App />,
  document.getElementById('app')
);