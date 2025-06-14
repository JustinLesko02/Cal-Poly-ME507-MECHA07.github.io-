/*
 * Interface.c
 *
 *  Created on: Jun 5, 2025
 *      Author: longe
 */

#include "Interface.h"

const bool DEMO = false; /**< when set true, feeder will create its own access point
						 when set false, feeder will connect to home wifi */
const bool FAST_START = true; /**< when set true, feeder will skip the network setup step
							   the esp wifi chip will automatically remember the last settings*/

const int MORE_VAL = 20; /**< the weight to increase the target when requested by user */
const int LESS_VAL = 10; /**< the weight to decrease the target when requested by user */

const char HTTP_HEADER[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "Content-Length: %d\r\n"
    "\r\n";

const char HTTP_CONTENT[] =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head><title>AUTOMATIC FEEDER</title></head>\n"
    "<body>\n"
	"  <h2>%s</h2>"
	"  <h2>TARGET WEIGHT:  %d</h2>"
    "  <h2>CURRENT WEIGHT: %d</h2>\n"
    "  <form method=\"POST\" action=\"/ENA\" style=\"margin-top: 50px;\">\n"
    "    <button type=\"submit\" style=\"font-size : 20px;\">TOGGLE ENABLE</button>\n"
    "  </form>\n"
	"  <form method=\"POST\" action=\"/FED\" style=\"margin-top: 100px;\">\n"
	"    <button type=\"submit\" style=\"font-size : 20px;\">FEED ONCE</button>\n"
	"  </form>\n"
	"  <form method=\"POST\" action=\"/MOR\" style=\"margin-top: 100px;\">\n"
	"    <button type=\"submit\" style=\"font-size : 20px;\">MAINTAIN MORE</button>\n"
	"  </form>\n"
	"  <form method=\"POST\" action=\"/LES\" style=\"margin-top: 25px;\">\n"
	"    <button type=\"submit\" style=\"font-size : 20px;\">MAINTAIN LESS</button>\n"
	"  </form>\n"
    "  <form method=\"POST\" action=\"/SCR\" style=\"margin-top: 100px;\">\n"
    "    <button type=\"submit\" style=\"font-size : 20px;\">ADVANCE SCREW</button>\n"
    "  </form>\n"
    "  <form method=\"POST\" action=\"/FLA\" style=\"margin-top: 25px;\">\n"
    "    <button type=\"submit\" style=\"font-size : 20px;\">CYCLE FLAP</button>\n"
    "  </form>\n"
    "</body>\n"
    "</html>\n";

const char HTTP_REDIRECT[] =
    "HTTP/1.1 302 Found\r\n"
    "Location: /\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "\r\n";

interface_struct_t interStruct;

void init_interface(UART_HandleTypeDef* uart1, UART_HandleTypeDef* wifi) {
	interStruct.uart1 = uart1;
	interStruct.wifi = wifi;

	interStruct.TS = 0;
	interStruct.initState = S0_DELAY;
	interStruct.prevInitState = S6_DONE;
	interStruct.offset = 0;

	// start the wifi server
	 HAL_UART_Receive_IT(interStruct.wifi, &interStruct.wifiChar, 1);
}

int start_interface() {

	if (interStruct.initState == S0_DELAY) { // wait a second to let everything start up
		  if (interStruct.prevInitState != interStruct.initState) {
			int len = sprintf(interStruct.uartOut, "WIFI STATE: 0\r\n");
			HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);

			interStruct.TS = HAL_GetTick();

			interStruct.prevInitState = interStruct.initState;
		  }
		  if ((HAL_GetTick() - interStruct.TS) > 1000) {
			  interStruct.initState = S1_RESET;
			  interStruct.TS = HAL_GetTick();
		  }

	  } else if (interStruct.initState == S1_RESET) { // reset the wifi chip
		  if (interStruct.prevInitState != interStruct.initState) {
			  int len = sprintf(interStruct.uartOut, "WIFI STATE: 1\r\n");
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);

			  interStruct.TS = HAL_GetTick();

			  len = sprintf(interStruct.wifiOut, "AT+RST\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  interStruct.prevInitState = interStruct.initState;
		  }

		  if ((HAL_GetTick() - interStruct.TS) > 1000) {
			  interStruct.initState = S2_MODE;
			  interStruct.TS = HAL_GetTick();
		  }

	  } else if (interStruct.initState == S2_MODE) { // set the wifi mode (access point vs client)
		  if (interStruct.prevInitState != interStruct.initState) {
			  int len = sprintf(interStruct.uartOut, "WIFI STATE: 2\r\n");
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);

			  interStruct.TS = HAL_GetTick();

			  if (!DEMO) {
				  int len = sprintf(interStruct.wifiOut, "AT+CWMODE=1\r\n");
				  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  } else {
				  int len = sprintf(interStruct.wifiOut, "AT+CWMODE=2\r\n");
				  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  }
			  interStruct.prevInitState = interStruct.initState;
		  }

		  if ((HAL_GetTick() - interStruct.TS) > 1000) {
			  interStruct.initState = S3_NETWORK;
			  interStruct.TS = HAL_GetTick();
		  }

	  } else if (interStruct.initState == S3_NETWORK) { // connect / set up the wifi network
		  if (interStruct.prevInitState != interStruct.initState) {
			  int len = sprintf(interStruct.uartOut, "WIFI STATE: 3\r\n");
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);

			  interStruct.TS = HAL_GetTick();

			  // if fast start is enabled, we skip this step and rely on the wifi module to remember the network
			  if (!FAST_START) {
				  if (!DEMO) {
					  len = sprintf(interStruct.wifiOut, "AT+CWJAP=\"arnoldPALMers\",\"narwalls\"\r\n");
					  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
				  } else {
					  len = sprintf(interStruct.wifiOut, "AT+CWSAP=\"FEEDER\",\"12345678\",1,2\r\n");
					  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
				  }
			  }
			  interStruct.prevInitState = interStruct.initState;
		  }

		  if (!FAST_START) {
			  if ((HAL_GetTick() - interStruct.TS) > 10000) {
				  interStruct.initState = S4_MUX;
				  interStruct.TS = HAL_GetTick();
			  }
		  } else {
			if ((HAL_GetTick() - interStruct.TS) > 1000) {
				interStruct.initState = S4_MUX;
				interStruct.TS = HAL_GetTick();
			  }
		  }

	  } else if (interStruct.initState == S4_MUX) { // configure simultaneous wifi connections
		  if (interStruct.prevInitState != interStruct.initState) {
			  int len = sprintf(interStruct.uartOut, "WIFI STATE: 4\r\n");
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);

			  len = sprintf(interStruct.wifiOut, "AT+CIPMUX=1\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  interStruct.prevInitState = interStruct.initState;
		  }

		  if ((HAL_GetTick() - interStruct.TS) > 2000) {
			  interStruct.initState = S5_SERVER;
			  interStruct.TS = HAL_GetTick();
		  }

	  } else if (interStruct.initState == S5_SERVER) { // start the wifi server on port 80
		  if (interStruct.prevInitState != interStruct.initState) {
			  int len = sprintf(interStruct.uartOut, "WIFI STATE: 5\r\n");
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);

			  len = sprintf(interStruct.wifiOut, "AT+CIPSERVER=1,80\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  interStruct.prevInitState = interStruct.initState;
		  }

		  if ((HAL_GetTick() - interStruct.TS) > 2000) {
			  interStruct.initState = S0_DELAY;
			  interStruct.TS = HAL_GetTick();
			  return 1;
		  }

	  }

	return 0;
}

void run_interface() {
	// check uart commands
	  if (HAL_UART_Receive(interStruct.uart1, &interStruct.uartChar, 1, 0) == HAL_OK) {
		  if ((interStruct.uartChar != 13) || (interStruct.uartChar != 10)) {
			  // echo back to user
			  HAL_UART_Transmit(interStruct.uart1, &interStruct.uartChar, 1, 100);
		  }

		  if (interStruct.uartChar == 13) { // enter for hello world
			  int len = sprintf(interStruct.uartOut, "HELLO WORLD!\r\n");
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);
		  }
		  if (interStruct.uartChar == 49) { // 1 reset wifi
			  int len = sprintf(interStruct.wifiOut, "AT+RST\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
		  }
		  if (interStruct.uartChar == 50) { // 2 set wifi mode
			  if (!DEMO) {
				  int len = sprintf(interStruct.wifiOut, "AT+CWMODE=1\r\n");
				  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  } else {
				  int len = sprintf(interStruct.wifiOut, "AT+CWMODE=2\r\n");
				  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  }
		  }

		  if (interStruct.uartChar == 51) { // 3 connect to wifi
			  if (!DEMO) {
				  //int numChars = sprintf(outBuf2, "AT+CWJAP=\"evansiphone\",\"bbbbbbbb\"\r\n");
				  int len = sprintf(interStruct.wifiOut, "AT+CWJAP=\"arnoldPALMers\",\"narwalls\"\r\n");
				  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  } else {
				  int len = sprintf(interStruct.wifiOut, "AT+CWSAP=\"FEEDER\",\"12345678\",1,2\r\n");
				  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
			  }

		  }
		  if (interStruct.uartChar == 52) { // 4 set connection type
			  int len = sprintf(interStruct.wifiOut, "AT+CIPMUX=1\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
		  }
		  if (interStruct.uartChar == 53) { // 5 start wifi server
			  int len = sprintf(interStruct.wifiOut, "AT+CIPSERVER=1,80\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
		  }

		  if (interStruct.uartChar == 54) { // 6 print current IP
			  int len = sprintf(interStruct.wifiOut, "AT+CIFSR\r\n");
			  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
		  }


		  if (interStruct.uartChar == 114) { // r read weight
			  int len = sprintf(interStruct.uartOut, ": WEIGHT: %d\r\n", get_weight());
			  HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);
		  }

		  if (interStruct.uartChar == 97) { // a toggle idle
			  enable_filler();
		  }

		  if (interStruct.uartChar == 115) { // s advance screw
			  advance_screw();
		  }

		  if (interStruct.uartChar == 100) { // d advance flap
			  cycle_flap();
		  }

	  }

	  // forward wifi data to uart
	  if (interStruct.wifiIndex2 < interStruct.wifiIndex) {
		  HAL_UART_Transmit(interStruct.uart1, &(interStruct.wifiIn[interStruct.wifiIndex2]), (interStruct.wifiIndex-interStruct.wifiIndex2), 100);
		  interStruct.wifiIndex2 = interStruct.wifiIndex;
	  }

	  // check wifi commands
	  if (interStruct.wifiIndex > 0) {

		  int wifiAddr = 0;

		  for (int i = 0; i < interStruct.wifiIndex-4; i++) {

			  // update the wifi address with the most recent channel
			  if (interStruct.wifiIn[i] == 43) { // +
				  if (interStruct.wifiIn[i+1] == 73) { // I
					  if (interStruct.wifiIn[i+2] == 80) { // P
						  if (interStruct.wifiIn[i+3] == 68) { // D
							  int nextAddr = 0;
							  // convert str to int
							  for (int j = i+5; j < interStruct.wifiIndex; j++) {
								  if (interStruct.wifiIn[j] == 44) { // ,
									  wifiAddr = nextAddr;

									  break;
								  } else {
									  nextAddr = 10*nextAddr+interStruct.wifiIn[j]-48;
								  }
							  }
						  }
					  }
				  }
			  }

			  if (interStruct.wifiIn[i] == 71) { // G
				  if (interStruct.wifiIn[i+1] == 69) { // E
					  if (interStruct.wifiIn[i+2] == 84) { // T

						  // return the website

						  // first determine what to display
						  char* enaStr;
						  if (get_enable_state() == 0) {
							  enaStr = "DISABLED";
						  } else if (get_enable_state() == 1) {
							  enaStr = "ENABLED";
						  } else if (get_enable_state() == -1) {
							  enaStr = "TIMOUT ERROR";
						  } else if (get_enable_state() == -2) {
							  enaStr = "LOAD CELL ERROR";
						  } else {
							  enaStr = "?";
						  }

						  // calculate length of content
						  int len = sprintf(interStruct.wifiOut2, HTTP_CONTENT, enaStr, get_target()-interStruct.offset, get_weight()-interStruct.offset);
						  // fill in header with length, calculate length of header
						  len = sprintf(interStruct.wifiOut, HTTP_HEADER, len);
						  // calculate length of content
						  int len2 = sprintf(interStruct.wifiOut, HTTP_CONTENT, enaStr, get_target()-interStruct.offset, get_weight()-interStruct.offset);
						  // prep esp module for data transmission
						  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len2+len);
						  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
						  HAL_Delay(100);
						  // calculate length of content
						  len = sprintf(interStruct.wifiOut2, HTTP_CONTENT, enaStr, get_target()-interStruct.offset, get_weight()-interStruct.offset);
						  // fill in header with length, calculate length of header
						  len = sprintf(interStruct.wifiOut, HTTP_HEADER, len);
						  // concat the header and the content in memory
						  len2 = sprintf(&interStruct.wifiOut[len], HTTP_CONTENT, enaStr, get_target()-interStruct.offset, get_weight()-interStruct.offset);
						  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len+len2, 100);
						  HAL_Delay(200);
						  // close the connection
						  len = sprintf(interStruct.wifiOut, "AT+CIPCLOSE=%d\r\n", wifiAddr);
						  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
						  // empty the buffer
						  interStruct.wifiIndex = 0;
						  interStruct.wifiIndex2 = 0;
					  }
				  }
			  }

			  if (interStruct.wifiIn[i] == 47) { // /
				  if (interStruct.wifiIn[i+1] == 69) { // E
					  if (interStruct.wifiIn[i+2] == 78) { // N
						  if (interStruct.wifiIn[i+3] == 65) { // A

							  enable_filler();

							  // set the calibration offset
							  int aveWeight = get_weight_filtered();
							  interStruct.offset = aveWeight;
							  set_target(aveWeight);

							  // send the redirect response over wifi
							  int len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  HAL_Delay(100);
							  len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  // reset the buffer
							  interStruct.wifiIndex = 0;
							  interStruct.wifiIndex2 = 0;
						  }
					  }
				  }
			  }

			  if (interStruct.wifiIn[i] == 47) { // /
				  if (interStruct.wifiIn[i+1] == 70) { // F
					  if (interStruct.wifiIn[i+2] == 69) { // E
						  if (interStruct.wifiIn[i+3] == 68) { // D

							  // increase the target weight
							  set_target(get_target() + MORE_VAL);
							  // set run once flag
							  run_once();

							  // send the redirect response over wifi
							  int len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  HAL_Delay(100);
							  len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  // reset the buffer
							  interStruct.wifiIndex = 0;
							  interStruct.wifiIndex2 = 0;
						  }
					  }
				  }
			  }

			if (interStruct.wifiIn[i] == 47) { // /
				  if (interStruct.wifiIn[i+1] == 77) { // M
					  if (interStruct.wifiIn[i+2] == 79) { // O
						  if (interStruct.wifiIn[i+3] == 82) { // R

							  // increase the target weight
							  set_target(get_target() + MORE_VAL);

							  // send the redirect response over wifi
							  int len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  HAL_Delay(100);
							  len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  // reset the buffer
							  interStruct.wifiIndex = 0;
							  interStruct.wifiIndex2 = 0;
						  }
					  }
				  }
			  }

			  if (interStruct.wifiIn[i] == 47) { // /
				  if (interStruct.wifiIn[i+1] == 76) { // L
					  if (interStruct.wifiIn[i+2] == 69) { // E
						  if (interStruct.wifiIn[i+3] == 83) { // S

							  // decrease the target weight
							  set_target(get_target() - LESS_VAL);

							  // send the redirect response over wifi
							  int len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  HAL_Delay(100);
							  len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  // reset the buffer
							  interStruct.wifiIndex = 0;
							  interStruct.wifiIndex2 = 0;
						  }
					  }
				  }
			  }

			  if (interStruct.wifiIn[i] == 47) { // /
				  if (interStruct.wifiIn[i+1] == 83) { // S
					  if (interStruct.wifiIn[i+2] == 67) { // C
						  if (interStruct.wifiIn[i+3] == 82) { // R
							  // advance the screw
							  advance_screw();

							  // send the redirect response over wifi
							  int len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  HAL_Delay(100);
							  len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  // reset the buffer
							  interStruct.wifiIndex = 0;
							  interStruct.wifiIndex2 = 0;
						  }
					  }
				  }
			  }

			  if (interStruct.wifiIn[i] == 47) { // /
				  if (interStruct.wifiIn[i+1] == 70) { // F
					  if (interStruct.wifiIn[i+2] == 76) { // L
						  if (interStruct.wifiIn[i+3] == 65) { // A
							  // run the flap
							  cycle_flap();

							  // send the redirect response over wifi
							  int len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  len = sprintf(interStruct.wifiOut, "AT+CIPSEND=%d,%d\r\n", wifiAddr, len);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  HAL_Delay(100);
							  len = sprintf(interStruct.wifiOut, HTTP_REDIRECT);
							  HAL_UART_Transmit(interStruct.wifi, interStruct.wifiOut, len, 100);
							  // reset the buffer
							  interStruct.wifiIndex = 0;
							  interStruct.wifiIndex2 = 0;
						  }
					  }
				  }
			  }
		  }
	  }

	  // clear wifi buffer if its full of junk
	  if (interStruct.wifiIndex > 90) {
		  interStruct.wifiIndex = 0;
		  interStruct.wifiIndex2 = 0;
	  }
}

void print_msg(char* msg) {
	int len = sprintf(interStruct.uartOut, msg);
	 HAL_UART_Transmit(interStruct.uart1, interStruct.uartOut, len, 100);
}

void wifi_interrupt(UART_HandleTypeDef *huart) {
	// store the char in the buffer
	interStruct.wifiIn[interStruct.wifiIndex] = interStruct.wifiChar;
	// increment the index
	if (interStruct.wifiIndex < 99) {
		interStruct.wifiIndex++;
	}
	// setup the next interrupt
	HAL_UART_Receive_IT(huart, &interStruct.wifiChar, 1);
}

int get_offset() {
	return interStruct.offset;
}

void set_offset(int offset) {
	interStruct.offset = offset;
}

