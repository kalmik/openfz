var path = require('path');
var webpack = require('webpack');

module.exports = {
  entry: {
    loader: './src/main.jsx',
    openfzUi: './src/openfz-ui.jsx'
  },
  output: {
    path: path.join(__dirname, 'dist/js'), 
    filename: 'bundle.[name].js' 
  },
  module: {
    loaders: [{
      test: /.jsx?$/,
      loader: 'babel-loader',
      exclude: /node_modules/,
      query: {
        presets: ['es2015', 'react']
      }
    }]
  },
};
