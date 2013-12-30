// Assembles packets in the specified buffer using the data provided.
// Call with remaining = 0 the first time, then update remaining with the return value.
// Calls the specified handler with each completed buffer. Packets are assumed to
// start with 3 consecutive bytes of 0xFE followed by an unsigned byte that specifies
// the packet type. Payload sizes for different packet types are hardcoded.
// Automatically aligns to packet boundaries when called with remaining = 0 or when
// an assembled packet has an unexpected header.
exports.ingest = function(data,buffer,remaining,handler) {
	var nextAvail = 0;
	while(nextAvail < data.length) {
		if(remaining == -3) {
			// The next byte is the packet type.
			type = data.readUInt8(nextAvail);
			if(type == 0x01) {
				// NB: hardcoded payload size for a DATA_PACKET
				remaining = 32;
			}
			else {
				console.log('Skipping unexpected packet type',type);
				return 0;
			}
			console.log('start packet',type,remaining);
			// Check that the buffer provided is big enough.
			if(remaining + 4 > buffer.length) {
				console.log('Skipping packet with payload overflow',remaining+4,buffer.length);
				return 0;
			}
			buffer[4] = type;
			nextAvail++;
		}
		else if(remaining <= 0) {
			// Look for a header byte.
			if(data[nextAvail] == 0xFE) {
				buffer[-remaining] = 0xFE;
				remaining -= 1;
			}
			else {
				// Forget any previously seen header bytes.
				remaining = 0;
			}
			nextAvail++;
		}
		else {
			var toCopy = Math.min(remaining,data.length-nextAvail);
			data.copy(buffer,buffer.length-remaining,nextAvail,nextAvail+toCopy);
			nextAvail += toCopy;
			remaining -= toCopy;
			if(remaining === 0) {
				// Calls the specified handler with the assembled packet.
				handler(buffer);
			}
		}
	}
	return remaining;
}
