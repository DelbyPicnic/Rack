class PortProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        var self = this;
        this.port.onmessage = (event) => {
            if (event.data.cmd == 'init') {
                var k = librack({
                    buffer: event.data.buffer,
                    PthreadWorkerInit: event.data.PthreadWorkerInit,
                    stackSize: event.data.stackSize,
                    stackBase: event.data.stackBase,
                    tempDoublePtr: event.data.tempDoublePtr,
                    TOTAL_MEMORY: event.data.TOTAL_MEMORY,
                    STATICTOP: event.data.STATICTOP,
                    DYNAMIC_BASE: event.data.DYNAMIC_BASE,
                    DYNAMICTOP_PTR: event.data.DYNAMICTOP_PTR,
                });
                self.lr = k;
            
            } else if (event.data.cmd == 'start') {
                self.addr = event.data.addr;
            
            } else if (event.data.cmd == 'stop') {
                self.addr = 0;
            }
        };
    }

    process(inputs, outputs, parameters) {
        if (!this.addr)
            return true;

        var out = outputs[0];
        var channel0 = out[0];
        var channel1 = out[1];
        var pData = this.addr;
        pData >>= 2;

        this.lr._processAudioJS(channel0.length);

        for (var i = 0; i < channel0.length; ++i) {
            channel0[i] = this.lr.HEAPF32[pData++];
            channel1[i] = this.lr.HEAPF32[pData++];
        }

        return true;
    }
}

registerProcessor('port-processor', PortProcessor);