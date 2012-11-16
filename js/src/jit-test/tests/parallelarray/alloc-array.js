load(libdir + "parallelarray-helpers.js");

function buildSimple() {

    assertParallelArrayModesCommute(["seq", "par"], function(m) {
        return new ParallelArray([256], function(i) {
            var x = [];
            for (var i = 0; i < 4; i++) {
                x[i] = i;
            }
            return x;
        }, m);
    });

    // Eventually, this should work, but right now it
    // bails out because we overflow the size of the array
    // assertParallelArrayModesCommute(["seq", "par"], function(m) {
    //     return new ParallelArray([256], function(i) {
    //         var x = [];
    //         for (var i = 0; i < 99; i++) {
    //             x[i] = i;
    //         }
    //         return x;
    //     }, m);
    // });
    
}

buildSimple();
