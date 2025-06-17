#!/usr/bin/env python3
"""
Implementación del protocolo SLIP (Serial Line Internet Protocol) RFC 1055
para envío y recepción de tramas de paquetes.
Compatible con la implementación de C++ SlipProcessor.

Constantes SLIP:
- SLIP_END     = 0xC0
- SLIP_ESC     = 0xDB
- SLIP_ESC_END = 0xDC
- SLIP_ESC_ESC = 0xDD
"""

class SlipProcessor:
    """Procesador de protocolo SLIP para codificación y decodificación de tramas"""
    
    SLIP_END     = 0xC0
    SLIP_ESC     = 0xDB
    SLIP_ESC_END = 0xDC
    SLIP_ESC_ESC = 0xDD
    
    def __init__(self):
        """Inicializar el procesador SLIP"""
        self.buffer = bytearray()
        self.escape_next = False
        self.packet_callback = None
        self.debug = False
    
    def set_packet_callback(self, callback):
        """
        Establecer función callback que se llama cuando se recibe un paquete completo
        
        Args:
            callback: Función que recibe (packet: bytes) como parámetro
        """
        self.packet_callback = callback
    
    def set_debug(self, debug):
        """Habilitar/deshabilitar debugging"""
        self.debug = debug
    
    @staticmethod
    def encode_slip(data):
        """
        Codificar datos usando protocolo SLIP (compatible con C++)
        
        Args:
            data (bytes): Datos a codificar
            
        Returns:
            bytes: Datos codificados con protocolo SLIP
        """
        if isinstance(data, str):
            data = data.encode('utf-8')
        elif isinstance(data, bytearray):
            data = bytes(data)
        
        encoded = bytearray()
        
        for byte in data:
            if byte == SlipProcessor.SLIP_END:
                encoded.append(SlipProcessor.SLIP_ESC)
                encoded.append(SlipProcessor.SLIP_ESC_END)
            elif byte == SlipProcessor.SLIP_ESC:
                encoded.append(SlipProcessor.SLIP_ESC)
                encoded.append(SlipProcessor.SLIP_ESC_ESC)
            else:
                encoded.append(byte)
        
        encoded.append(SlipProcessor.SLIP_END)
        return bytes(encoded)
    
    def on_data_received(self, data):
        """
        Procesar datos recibidos y extraer paquetes SLIP (compatible con C++)
        
        Args:
            data (bytes): Datos recibidos para procesar
        """
        if isinstance(data, str):
            data = data.encode('utf-8')
        
        for byte in data:
            if self.debug:
                print(f"Decoding byte: 0x{byte:02x}")
                
            if self.escape_next:
                if byte == self.SLIP_ESC_END:
                    self.buffer.append(self.SLIP_END)
                elif byte == self.SLIP_ESC_ESC:
                    self.buffer.append(self.SLIP_ESC)
                else:
                    self.buffer.clear()
                self.escape_next = False
            else:
                if byte == self.SLIP_END:
                    if len(self.buffer) > 0:
                        packet = bytes(self.buffer)
                        if self.debug:
                            print(f"Packet received: {packet.hex()}")
                        self.buffer.clear()
                        if self.packet_callback:
                            self.packet_callback(packet)
                        return packet
                elif byte == self.SLIP_ESC:
                    self.escape_next = True
                else:
                    self.buffer.append(byte)
    
    def process_data(self, data):
        """Alias para on_data_received para compatibilidad"""
        return self.on_data_received(data)
    
    def reset(self):
        """Reiniciar el estado del procesador"""
        self.buffer.clear()
        self.escape_next = False
