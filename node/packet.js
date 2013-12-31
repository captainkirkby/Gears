// Assembles packets using the data provided. Packets are assumed to
// start with 3 consecutive bytes of 0xFE followed by an unsigned byte that specifies
// the packet type. Payload sizes for different packet types are hardcoded.
// Automatically aligns to packet boundaries when called with remaining = 0 or when
// an assembled packet has an unexpected header.

exports.Assembler = Assembler;

function Assembler(maxPayloadSize) {
	this.remaining = 0;
	this.packetType = null;
	this.buffer = new Buffer(maxPayloadSize);
}

Assembler.prototype.ingest = function(data,handler) {
	var nextAvail = 0;
	while(nextAvail < data.length) {
		if(this.remaining == -3) {
			// We have already seen 3 consecutive header bytes, so the next byte is the packet type.
			this.packetType = data.readUInt8(nextAvail);
			// NB: payload sizes for each packet type are hardcoded here.
			if(this.packetType == 0x00) {
				this.remaining = 27;
			}
			else if(this.packetType == 0x01) {
				this.remaining = 32;
			}
			else {
				console.log('Skipping unexpected packet type',this.packetType);
				this.remaining = 0;
				return;
			}
			console.log('start packet',this.packetType,this.remaining);
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
			if(data[nextAvail] == 0xFE) {
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
