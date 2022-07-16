#ifndef CUSTOM_PDO_NAME_H
#define CUSTOM_PDO_NAME_H

//-------------------------------------------------------------------//
//                                                                   //
//     This file has been created by the Easy Configurator tool      //
//                                                                   //
//     Easy Configurator project test1.prj
//                                                                   //
//-------------------------------------------------------------------//


#define CUST_BYTE_NUM_OUT	8
#define CUST_BYTE_NUM_IN	8
#define TOT_BYTE_NUM_ROUND_OUT	8
#define TOT_BYTE_NUM_ROUND_IN	8


typedef union												//---- output buffer ----
{
	uint8_t  Byte [TOT_BYTE_NUM_ROUND_OUT];
	struct
	{
		uint32_t    uint32_out;
		uint16_t    uint16_out;
		uint8_t     uint8_out;
		int8_t      int8_out;
	}Cust;
} PROCBUFFER_OUT;


typedef union												//---- input buffer ----
{
	uint8_t  Byte [TOT_BYTE_NUM_ROUND_IN];
	struct
	{
		uint32_t    uint32_in;
		uint16_t    uint16_in;
		uint8_t     uint8_in;
		int8_t      int8_in;
	}Cust;
} PROCBUFFER_IN;

#endif