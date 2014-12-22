// Assembles packets using the data provided. Packets are assumed to
// start with repeated identical header bytes followed by an unsigned byte that specifies
// the packet type. Payload sizes for different packet types are hardcoded.
// Automatically aligns to packet boundaries on startup or after a framing error.

exports.Assembler = Assembler;

// Initializes a new Assembler for packets that begin with headerSize repetitions of headerByte
// followed by a packet type byte and a payload. The payload size cannot exceed maxPayloadSize
// and the expected payload size for different packet types that we initially accept are specified
// in the payloadSizes dictionary. Additional packet types can be registered in the handler
// passed to our ingest function (see below).
function Assembler(headerByte,headerSize,maxPayloadSize,payloadSizes,debugLevel) {
	this.remaining = 0;
	this.packetType = null;
	this.headerByte = headerByte;
	this.headerSize = headerSize;
	this.maxPayloadSize = maxPayloadSize;
	this.payloadSizes = payloadSizes;
	for(var type in payloadSizes) {
		if(payloadSizes[type] > maxPayloadSize) {
			throw new Error('Assembler: Packet payload size exceeds maxPayloadSize');
		}
	}
	this.debugLevel = debugLevel;
	if(this.debugLevel > 0) {
		console.log('Initializing packet assembler with max payload size of',maxPayloadSize);
	}
	this.buffer = new Buffer(maxPayloadSize);
	this.lastBuffer = new Buffer(maxPayloadSize);
}

Assembler.prototype.addPacketType = function(type,size) {
	if(!(type in this.payloadSizes)) {
		if(this.debugLevel > 0) {
			console.log('registering new packet type',type,'with payload size',size);
		}
		this.payloadSizes[type] = size;
		if(size > this.maxPayloadSize) {
			throw new Error('Assembler: new packet payload size exceeds maxPayloadSize');
		}
	}
}

Assembler.prototype.ingest = function(data,handler) {
	if(this.debugLevel > 1) {
		console.log('with',this.remaining,'remaining, received',data.length,data);
	}
	var nextAvail = 0;
	while(nextAvail < data.length) {
		if(this.debugLevel > 1) {
			console.log('remaining',this.remaining,'ingesting from',nextAvail,'of',
				data.length,data.slice(nextAvail));
		}
		if(this.remaining == -this.headerSize) {
			// We have already seen all the header bytes, so the next byte is the packet type.
			this.packetType = data.readUInt8(nextAvail);
			// Is this a packet type we know about?
			if(this.packetType in this.payloadSizes) {
				this.remaining = this.payloadSizes[this.packetType];
				if(this.debugLevel > 1) {
					console.log('start packet',this.packetType,'with payload size',this.remaining);
				}
			}
			else {
				console.log('Skipping unexpected packet type',this.packetType);
				this.remaining = 0;
				return;
			}
			// Check that our buffer is big enough.
			if(this.remaining > this.buffer.length) {
				console.log('Skipping packet with payload overflow',this.remaining,this.buffer.length);
				this.remaining = 0;
				return;
			}
			nextAvail++;
		}
		else if(this.remaining <= 0) {
			// Look for a header byte.
			if(data[nextAvail] == this.headerByte) {
				this.remaining -= 1;
			}
			else {
				if(this.debugLevel > 0) {
					console.log('Skipping unexpected padding');
					console.log('Last Buffer:');
					console.log(JSON.stringify(this.lastBuffer));
					throw new Error('Assembler: unexpected padding');
				}
				// Forget any previously seen header bytes.
				this.remaining = 0;
			}
			nextAvail++;
		}
		else {
			var toCopy = Math.min(this.remaining,data.length-nextAvail);
			var psize = this.payloadSizes[this.packetType];
			var offset = psize - this.remaining;
			data.copy(this.buffer,offset,nextAvail,nextAvail+toCopy);
			nextAvail += toCopy;
			this.remaining -= toCopy;
			if(this.remaining === 0) {
				if(this.debugLevel > 0) {
					console.log('assembled type',this.packetType,this.buffer.slice(0,psize));
				}
				// Stores last buffer
				this.lastBuffer = this.buffer.slice(0,psize);
				// Calls the specified handler with the assembled packet.
				handler(this.packetType,this.buffer.slice(0,psize));
			}
		}
	}
}
