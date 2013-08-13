// |jit-test| error:TypeError;

if (!this.hasOwnProperty("Type"))
  throw new TypeError();

var Color = new StructType({r: uint8, g: uint8, b: uint8});
var white2 = new Color({r: 255, toString: null, b: 253});

