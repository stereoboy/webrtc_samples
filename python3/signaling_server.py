import eventlet
import socketio

import config
import numpy as np
import time
import collections

import logging

logger = logging.getLogger('Signaling.Server')
logger.setLevel(logging.INFO)
handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter(fmt='%(asctime)s:[%(levelname)s][0x%(thread)X][%(name)s] (%(funcName)s:%(lineno)d) %(message)s'))
logger.addHandler(handler)
# logging.basicConfig(level=logging.WARNING, format='%(asctime)s:[%(levelname)s][%(process)d][%(name)s] - %(funcName)s:%(lineno)d - %(message)s')


# sio = socketio.Server(logger=logger, engineio_logger=True, max_http_buffer_size=480*640*4*2)
sio = socketio.Server(logger=False, engineio_logger=False, max_http_buffer_size=480*640*4*2)
app = socketio.WSGIApp(sio, static_files={
    '/': {'content_type': 'text/html', 'filename': 'index.html'}
})

Peer = collections.namedtuple('Peer', ['sid', 'host', 'port', 'room', 'ice', 'sdp'])

peers = []

PEER_DATACHANNEL='peer_datachannel'
flag_peer_connection = False

def get_counter_sid(sid):
    # print(peers)
    # print("sid: {}".format(sid))
    for p in peers:
        if p.sid == sid:
            continue
        else:
            # print("p.sid: {}".format(p.sid))
            return p.sid
    raise ValueError

@sio.on('connect')
def connect(sid, environ):
    global flag_peer_connection
    global count, elapsed_time_queue, start
    logger.info('>>> connect {}'.format(sid))
    for k, v in environ.items():
        logger.info('\t{}:{}'.format(k, v))

    count = 0
    elapsed_time_queue = collections.deque(maxlen=100)
    start = time.time()

    if flag_peer_connection:
        logger.error('Peer connection already established. Additional connections is not allowed.')
        sio.disconnect(sid)
        return

    p = Peer(sid=sid, host=environ['REMOTE_ADDR'], port=environ['REMOTE_PORT'], room=PEER_DATACHANNEL, ice=None, sdp=None)
    peers.append(p)

    sio.enter_room(sid, PEER_DATACHANNEL)

    if len(peers) == 1:
        logger.info('>>> peer_#0 waiting for another peer')
        res = sio.emit('set-as-caller', to=sid)
        logger.info('\t - {}'.format(res))
    elif len(peers) == 2:
        logger.info('>>> peer_#1 arrived. Ready to test P2P.')
        res = sio.emit('set-as-callee', to=sid)
        logger.info('\t - {}'.format(res))
        flag_peer_connection = True
        logger.info('{}'.format(peers))

        sio.emit('ready', room=PEER_DATACHANNEL)


@sio.on('query-peer-type')
def query_peer_type(sid, data):

    if sid == peers[0].sid:
        # res = sio.call('set-as-caller', "dummy", to=sid)
        # logger.info('{}'.format(res))
        logger.info("caller")
        return "caller"
    elif sid == peers[1].sid:
        # res = sio.call('set-as-callee', "dummy", to=sid)
        # logger.info('{}'.format(res))
        logger.info("callee")
        return "callee"
    else:
        logger.error('Unknown peer: {}'.format(sid))
        return "unknown"

@sio.on('ice')
def on_ice(sid, data):
    sio.emit('start', msg, room=PEER_DATACHANNEL, skip_sid=sid)
    pass

@sio.on('offer')
def on_sdp(sid, sdp):
    logger.info("SDP:\n{}".format(sdp))
    ret = sio.call('offer', sdp, to=get_counter_sid(sid))
    return ret

@sio.on('answer')
def on_sdp(sid, sdp):
    logger.info("SDP:\n{}".format(sdp))
    ret = sio.call('answer', sdp, to=get_counter_sid(sid))
    return ret

# @sio.on('message')
# def on_message(sid, data):
#     global count, elapsed_time_queue, start
#     logger.debug('>>> \tmessage: len(data)={}'.format(len(data)))
#     end = time.time()
#     elapsed = end - start
#     start = time.time()
#     elapsed_time_queue.append(elapsed)
#     count += 1
#     if count%100 == 0:
#         logger.info("\t{} fps".format(1.0/np.mean(elapsed_time_queue)))
#     return True

@sio.on('disconnect')
def disconnect(sid):
    logger.info('>>> disconnect: {}'.format(sid))
    pass

if __name__ == '__main__':
    logger.info('Signaling.Server')
    eventlet.wsgi.server(eventlet.listen(('', config.PORT)), app)