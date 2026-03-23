var Module = { 'preRun': function() {

var firebirdCanvas = null;

initLCD = function()
{
    if (firebirdCanvas) {
        return firebirdCanvas;
    }

    var c = Module.canvas || document.getElementById("canvas");
    if (!c) {
        c = document.querySelector("#screen canvas, #display canvas, .screen canvas, canvas");
    }
    if (!c) {
        c = document.createElement("canvas");
        c.id = "canvas";
        var host = document.getElementById("screen") || document.getElementById("display") || document.body;
        host.appendChild(c);
    }

    var w = 320;
    var h = 240;
    c.width = w;
    c.height = h;

    var ctx = c.getContext('2d');
    var imageData = ctx.getImageData(0, 0, w, h);
    var bufSize = w * h * 4;
    var bufPtr = Module['_malloc'](bufSize);
    var buf = new Uint8Array(Module['HEAPU8']['buffer'], bufPtr, bufSize);

    var wrappedPaint = Module['cwrap']('paintLCD', 'void', ['number']);
    repaint = function() {
        wrappedPaint(buf.byteOffset);
        imageData.data.set(buf);
        ctx.putImageData(imageData, 0, 0);
        window.requestAnimationFrame(repaint);
    };
    repaint();
    firebirdCanvas = c;
    return c;
}

fileLoaded = function(event, filename)
{
    if (event.target.readyState == FileReader.DONE)
        FS.writeFile(filename, new Uint8Array(event.target.result), {encoding: 'binary'});
}

fileLoad = function(event, filename)
{
    var file = event.target.files[0];

    if(!file)
        return FS.unlink(filename);

    var reader = new FileReader();
    reader.onloadend = function(event)
        {
            fileLoaded(event, filename);
        };

    if (file.webkitSlice)
        var blob = file.webkitSlice(0, file.size);
    else if (file.mozSlice)
        var blob = file.mozSlice(0, file.size);
    else
        var blob = file.slice(0, file.size);

    reader.readAsArrayBuffer(file);
}

startEmulation = function()
{
    initLCD();

    var fileExists = function(path) {
        try {
            FS.stat(path);
            return true;
        } catch (e) {
            return false;
        }
    };

    var ensureFile = function(targetPath, candidatePaths) {
        if (fileExists(targetPath)) {
            return true;
        }

        for (var i = 0; i < candidatePaths.length; i++) {
            var sourcePath = candidatePaths[i];
            if (!fileExists(sourcePath)) {
                continue;
            }

            FS.writeFile(targetPath, FS.readFile(sourcePath), { encoding: 'binary' });
            return true;
        }

        return false;
    };

    var hasBoot1 = ensureFile("boot1.img", ["/roms/boot1.img", "/roms/boot1.img.tns"]);
    var hasFlash = ensureFile("flash.img", ["/roms/flash.img", "/roms/flash"]);

    if (!hasBoot1 || !hasFlash) {
        alert("ROM files missing. Please choose Boot1 and Flash files, or preload them in /roms.");
        return;
    }

    return Module.callMain();
}

} // preRun function
} // Module
