const DEFAULT_DURATION = 1200;


const producer = getPoint('icon-producer');
const primary = getPoint('icon-primary');
const replica1 = getPoint('icon-replica-1');
const replica2 = getPoint('icon-replica-2');
const replica3 = getPoint('icon-replica-3');
const consumer = getPoint('icon-consumer');

const message = getPoint('icon-message');
const signalTickFromReplica3 = getPoint('icon-signal-tick');
signalTickFromReplica3.node.style.opacity = "0";

function move(node, tl, to, delay, animateOpacity) {
  const additionalAnimation = animateOpacity ? {
    opacity: [
      { value: 1, duration: 1000 },
      { value: 0, duration: 500 }
    ]
  } : {};
  tl.add({
    targets: node.query_id,
    translateX: to.x - node.x,
    translateY: to.y - node.y,
    ...additionalAnimation
  }, delay);
}

const tl = anime.timeline({
  duration: DEFAULT_DURATION,
  loop: true,
  easing: "linear",
});

move(message, tl, primary);

const msgToReplica1 = cloneNode(message, "message-to-replica-1", primary);
move(msgToReplica1, tl, replica1, undefined, true);
const signalTickFromReplica1 = cloneNode(signalTickFromReplica3, "icon-signal-tick-1", replica1);
move(signalTickFromReplica1, tl, primary, undefined,true);

const msgToReplica3 = cloneNode(message, "message-to-replica-3", primary);
move(msgToReplica3, tl, replica3, "-=2800", true);

move(signalTickFromReplica3, tl, primary, "-=1300", true);

const msgToReplica2 = cloneNode(message, "message-to-replica-2", primary);
move(msgToReplica2, tl, replica2, "-=2800", true);
const signalTickFromReplica2 = cloneNode(signalTickFromReplica3, "icon-signal-tick-3", replica2);
move(signalTickFromReplica2, tl, primary, "-=1300", true);

move(message, tl, replica3);
move(message, tl, consumer);
