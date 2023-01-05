{
    const start = process.hrtime();
    var n = 0;
    for (var i = 0; i < 100000; i = i + 1) {
        for (var j = 0; j < i; j = j + 1) {
            n = n + j;
        }
    }

    const end = process.hrtime();

    console.log(n);
    console.log(end[0] - start[0] + end[1] / 1000000000 - start[1] / 1000000000);

}
