/*
 * libwebsockets-test-client - libwebsockets test implementation
 *
 * Copyright (C) 2011 Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "../lib/libwebsockets.h"
#include <poll.h>

static unsigned int opts;

/*
 * This demo shows how to connect multiple websockets simultaneously to a
 * websocket server (there is no restriction on their having to be the same
 * server just it simplifies the demo).
 *
 *  dumb-increment-protocol:  we connect to the server and print the number
 *				we are given
 *
 *  lws-mirror-protocol: draws random circles, which are mirrored on to every
 *				client (see them being drawn in every browser
 *				session also using the test server)
 */

enum demo_protocols {

	PROTOCOL_DUMB_INCREMENT,
	PROTOCOL_LWS_MIRROR,

	/* always last */
	DEMO_PROTOCOL_COUNT
};


/* dumb_increment protocol */

static int
callback_dumb_increment(struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	switch (reason) {

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		fprintf(stderr, "rx %d '%s'\n", (int)len, (char *)in);
		break;

	default:
		break;
	}

	return 0;
}


/* lws-mirror_protocol */


static int
callback_lws_mirror(struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
						  LWS_SEND_BUFFER_POST_PADDING];
	int l;

	switch (reason) {

	case LWS_CALLBACK_CLIENT_ESTABLISHED:

		/*
		 * start the ball rolling,
		 * LWS_CALLBACK_CLIENT_WRITEABLE will come next service
		 */

		libwebsocket_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
/*		fprintf(stderr, "rx %d '%s'\n", (int)len, (char *)in); */
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:

		l = sprintf((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING],
					"c #%06X %d %d %d;",
					(int)random() & 0xffffff,
					(int)random() % 500,
					(int)random() % 250,
					(int)random() % 24);

		libwebsocket_write(wsi,
		   &buf[LWS_SEND_BUFFER_PRE_PADDING], l, opts | LWS_WRITE_TEXT);

		/* get notified as soon as we can write again */

		libwebsocket_callback_on_writable(wsi);

		/*
		 * without at least this delay, we choke the browser
		 * and the connection stalls, despite we now take care about
		 * flow control
		 */

		usleep(200);
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {

	[PROTOCOL_DUMB_INCREMENT] = {
		.name = "dumb-increment-protocol",
		.callback = callback_dumb_increment,
	},
	[PROTOCOL_LWS_MIRROR] = {
		.name = "lws-mirror-protocol",
		.callback = callback_lws_mirror,
	},
	[DEMO_PROTOCOL_COUNT] = {  /* end of list */
		.callback = NULL
	}
};

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ NULL, 0, 0, 0 }
};


int main(int argc, char **argv)
{
	int n = 0;
	int port = 7681;
	int use_ssl = 0;
	struct libwebsocket_context *context;
	const char *address = argv[1];
	struct libwebsocket *wsi_dumb;
	struct libwebsocket *wsi_mirror;

	fprintf(stderr, "libwebsockets test client\n"
			"(C) Copyright 2010 Andy Green <andy@warmcat.com> "
						    "licensed under LGPL2.1\n");

	if (argc < 2)
		goto usage;

	optind++;

	while (n >= 0) {
		n = getopt_long(argc, argv, "khsp:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
		case 's':
			use_ssl = 2; /* 2 = allow selfsigned */
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'k':
			opts = LWS_WRITE_CLIENT_IGNORE_XOR_MASK;
			break;
		case 'h':
			goto usage;
		}
	}

	/*
	 * create the websockets context.  This tracks open connections and
	 * knows how to route any traffic and which protocol version to use,
	 * and if each connection is client or server side.
	 *
	 * For this client-only demo, we tell it to not listen on any port.
	 */

	context = libwebsocket_create_context(CONTEXT_PORT_NO_LISTEN,
					      protocols, NULL, NULL, -1, -1, 0);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}


	/* create a client websocket using dumb increment protocol */

	wsi_dumb = libwebsocket_client_connect(context, address, port, use_ssl,
			"/", libwebsocket_canonical_hostname(context), "origin",
				       protocols[PROTOCOL_DUMB_INCREMENT].name);

	if (wsi_dumb == NULL) {
		fprintf(stderr, "libwebsocket dumb connect failed\n");
		return -1;
	}

	/* create a client websocket using mirror protocol */

	wsi_mirror = libwebsocket_client_connect(context, address, port,
	     use_ssl,  "/", libwebsocket_canonical_hostname(context), "origin",
				       protocols[PROTOCOL_LWS_MIRROR].name);

	if (wsi_mirror == NULL) {
		fprintf(stderr, "libwebsocket dumb connect failed\n");
		return -1;
	}

	fprintf(stderr, "Websocket connections opened\n");

	/*
	 * sit there servicing the websocket context to handle incoming
	 * packets, and drawing random circles on the mirror protocol websocket
	 */

	n = 0;
	while (n >= 0)
		n = libwebsocket_service(context, 1000);

	libwebsocket_context_destroy(context);

	return 0;

usage:
	fprintf(stderr, "Usage: libwebsockets-test-client "
					     "<server address> [--port=<p>] "
					     "[--ssl]\n");
	return 1;
}