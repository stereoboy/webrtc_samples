import sys
import argparse
import pathlib

import eventlet
import socketio

import config
import time
import collections

import logging

logger = logging.getLogger("Signaling.Server")
logger.setLevel(logging.INFO)
handler = logging.StreamHandler()
handler.setFormatter(
    logging.Formatter(
        fmt="%(asctime)s:[%(levelname)s][0x%(thread)X][%(name)s] (%(funcName)s:%(lineno)d) %(message)s"
    )
)
logger.addHandler(handler)
# logging.basicConfig(level=logging.WARNING, format='%(asctime)s:[%(levelname)s][%(process)d][%(name)s] - %(funcName)s:%(lineno)d - %(message)s')


class SignalingServer:
    def __init__(self):
        self.sio = socketio.Server(
            logger=False, engineio_logger=False, max_http_buffer_size=480 * 640 * 4 * 2
        )
        self.app = socketio.WSGIApp(
            self.sio,
            static_files={"/": {"content_type": "text/html", "filename": "index.html"}},
        )

        self.PEER_DATACHANNEL = "peer_datachannel"
        self.flag_peer_connection = False

        self.peers = []

        self.Peer = collections.namedtuple("Peer", ["sid", "host", "port", "room", "ice", "sdp"])

        self.count = 0
        self.elapsed_time_queue = collections.deque(maxlen=100)
        self.start = time.time()

    def get_counter_sid(self, sid):
        for p in self.peers:
            if p.sid == sid:
                continue
            else:
                return p.sid
        raise ValueError

    def setupt_callbacks(self):

        @self.sio.on("connect")
        def connect(sid, environ):
            logger.info(">>> connect: {} {}".format(sid, environ))
            for k, v in environ.items():
                logger.info("\t{}:{}".format(k, v))

            if self.flag_peer_connection:
                logger.error(
                    "Peer connection already established. Additional connections is not allowed."
                )
                self.sio.disconnect(sid)
                return

            p = self.Peer(
                sid=sid,
                host=environ["REMOTE_ADDR"],
                port=environ["REMOTE_PORT"],
                room=self.PEER_DATACHANNEL,
                ice=None,
                sdp=None,
            )
            self.peers.append(p)

            self.sio.enter_room(sid, self.PEER_DATACHANNEL)

            if len(self.peers) == 1:
                logger.info(">>> peer_#0 waiting for another peer")
                res = self.sio.emit("set-as-caller", to=sid)
                logger.info("\t - {}".format(res))
            elif len(self.peers) == 2:
                logger.info(">>> peer_#1 arrived. Ready to test P2P.")
                res = self.sio.emit("set-as-callee", to=sid)
                logger.info("\t - {}".format(res))
                self.flag_peer_connection = True
                logger.info("{}".format(self.peers))

                self.sio.emit("ready", room=self.PEER_DATACHANNEL)

        @self.sio.on("query-peer-type")
        def query_peer_type(sid, data):
            if sid == self.peers[0].sid:
                logger.info("caller")
                return "caller"
            elif sid == self.peers[1].sid:
                logger.info("callee")
                return "callee"
            else:
                logger.error("Unknown peer: {}".format(sid))
                return "unknown"

        @self.sio.on("ice")
        def on_ice(sid, ice):
            logger.info("ICE:\n{}".format(ice))
            ret = self.sio.call("ice", ice, to=self.get_counter_sid(sid))
            return ret


        @self.sio.on("offer")
        def on_sdp(sid, sdp):
            logger.info("SDP:\n{}".format(sdp))
            ret = self.sio.call("offer", sdp, to=self.get_counter_sid(sid))
            return ret


        @self.sio.on("answer")
        def on_sdp(sid, sdp):
            logger.info("SDP:\n{}".format(sdp))
            ret = self.sio.call("answer", sdp, to=self.get_counter_sid(sid))
            return ret

        @self.sio.on("disconnect")
        def disconnect(sid):
            logger.info(">>> disconnect: {}".format(sid))
            pass

        @self.sio.on("message")
        def on_message(sid, data):
            logger.debug(">>> \tmessage: len(data)={}".format(len(data)))
            end = time.time()
            elapsed = end - self.start
            self.start = time.time()
            self.elapsed_time_queue.append(elapsed)
            self.count += 1
            if self.count % 100 == 0:
                logger.info("\t{} fps".format(1.0 / np.mean(self.elapsed_time_queue)))
            return True

def main():

    parser = argparse.ArgumentParser(description='"This is a Test Signaling Server')
    parser.add_argument("--ssl", action="store_true", help="Use SSL")
    options = parser.parse_args(sys.argv[1:])

    logger.info("Signaling.Server with options: {}".format(options))

    signaling_server = SignalingServer()
    signaling_server.setupt_callbacks()

    certfile = pathlib.Path(__file__).with_name("server.crt")
    keyfile = pathlib.Path(__file__).with_name("server.key")


    if options.ssl:
        eventlet.wsgi.server(
            eventlet.wrap_ssl(
                eventlet.listen(("", config.PORT)),
                certfile=certfile,
                keyfile=keyfile,
                server_side=True,
            ),
            signaling_server.app,
        )
    else:
        eventlet.wsgi.server(
            eventlet.listen(("", config.PORT)),
            signaling_server.app)

if __name__ == "__main__":
    main()
