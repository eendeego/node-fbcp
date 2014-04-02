var fbcp = module.exports = require('./build/Release/fbcp.node');

var nativeStartup = fbcp.startup;
fbcp.startup = function (options) {
  nativeStartup(options);
  fbcp.copyBuffers = options.rescale ? fbcp.fullScreenCopy : fbcp.regionCopy;
};
