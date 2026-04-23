const Parser = require('tree-sitter');
const fs = require('fs');

// Load the RST grammar
const RST = require('./bindings/node');
const parser = new Parser();
parser.setLanguage(RST);

// Read the test file
const code = fs.readFileSync('./test_bullet.rst', 'utf8');

// Parse and show the tree
const tree = parser.parse(code);
console.log(tree.rootNode.toString());
