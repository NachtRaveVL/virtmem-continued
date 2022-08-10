#include "spiram.h"

//! @cond HIDDEN_SYMBOLS

namespace virtmem {

inline uint8_t SPISerialRam::sendByteMore(uint8_t byte)
{
#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    SPIFIFO.write(byte, SPI_CONTINUE);
    return SPIFIFO.read();
#else
    return SPI.transfer(byte);
#endif
}

inline uint8_t SPISerialRam::sendByteNoMore(uint8_t byte)
{
#ifdef VIRTMEM_SPIRAM_USESPIFIFO
    SPIFIFO.write(byte, 0);
    const uint8_t ret = SPIFIFO.read();
#else
    const uint8_t ret = SPI.transfer(byte);

    digitalWrite(_sramCSPin, HIGH);
#endif

    SPI.endTransaction();

    return ret;
}

}
