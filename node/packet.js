// Assembles packets using the data provided. Packets are assumed to
// start with repeated identical header bytes followed by an unsigned byte that specifies
// the packet type. Payload sizes for different packet types are hardcoded.
// Automatically aligns to packet boundaries on startup or after a framing error.

exports.Assembler = Assembler;

function Assembler(maxPayloadSize,headerByte,headerSize,payloadSizes) {
	this.remaining = 0;
	this.packetType = null;
	this.buffer = new Buffer(maxPayloadSize);
	this.headerByte = headerByte;
	this.headerSize = headerSize;
	this.payloadSizes = payloadSizes;
}

Assembler.prototype.ingest = function(data,handler) {
	var nextAvail = 0;
	while(nextAvail < data.length) {
		if(this.remaining == -this.headerSize) {
			// We have already seen all the header bytes, so the next byte is the packet type.
			this.packetType = data.readUInt8(nextAvail);
			// Is this a packet type we know about?
			if(this.packetType in this.payloadSizes) {
				this.remaining = this.payloadSizes[this.packetType];
				console.log('start packet',this.packetType,'with payload size',this.remaining);
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
				// Forget any previously seen header bytes.
				this.remaining = 0;
			}
			nextAvail++;
		}
		else {
			var toCopy = Math.min(this.remaining,data.length-nextAvail);
			data.copy(this.buffer,this.buffer.length-this.remaining,nextAvail,nextAvail+toCopy);
			nextAvail += toCopy;
			this.remaining -= toCopy;
			if(this.remaining === 0) {
				// Calls the specified handler with the assembled packet.
				handler(this.packetType,this.buffer);
			}
		}
	}	
}
