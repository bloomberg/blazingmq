const DEFAULT_DURATION = 1500;

const svg = document.querySelector("svg");
const tl = anime.timeline({
  duration: DEFAULT_DURATION,
  loop: true,
  easing: "linear",
});

const icon_message = `#icon-message`;
const icon_message_el = document.querySelector(icon_message);
icon_message_el.style.display = "none";

function journey(producerIndex, consumerIndex) {
  const msg = icon_message_el.cloneNode();
  msg.innerHTML = icon_message_el.innerHTML;
  msg.style.transformBox = "fill-box";
  msg.style.transformOrigin = "15px 15px";
  msg.style.display = "inherit";
  svg.appendChild(msg);

  const tl = anime.timeline({
    duration: 1200,
    easing: "linear",
    complete: () => {
      msg.remove();
    },
  });

  tl.add({
    duration: 0,
    targets: msg,
    translateX: POSITIONS.PRODUCER[producerIndex].x,
    translateY: POSITIONS.PRODUCER[producerIndex].y,
    scale: 0,
  });
  tl.add({
    duration: 100,
    targets: msg,
    translateX: POSITIONS.PRODUCER[producerIndex].x,
    translateY: POSITIONS.PRODUCER[producerIndex].y,
    scale: 1,
  });
  tl.add({
    targets: msg,
    translateX: POSITIONS.QUEUE.START.x,
    translateY: POSITIONS.QUEUE.START.y,
    opacity: 1,
  });
  tl.add({
    targets: msg,
    translateX: POSITIONS.QUEUE.END.x,
  });
  tl.add({
    targets: msg,
    translateX: POSITIONS.CONSUMER[consumerIndex].x,
    translateY: POSITIONS.CONSUMER[consumerIndex].y,
    scale: 1,
  });
  tl.add({
    duration: 100,
    targets: msg,
    translateX: POSITIONS.CONSUMER[consumerIndex].x,
    translateY: POSITIONS.CONSUMER[consumerIndex].y,
    scale: 0,
  });
}

const POSITIONS = {
  PRODUCER: [
    { x: 0, y: 0 },
    { x: 0, y: 92 },
    { x: 0, y: 184 },
    { x: 0, y: 276 },
  ],
  CONSUMER: [
    { x: 763, y: 0 },
    { x: 763, y: 92 },
    { x: 763, y: 184 },
    { x: 763, y: 276 },
  ],
  QUEUE: {
    START: { x: 200, y: 140 },
    END: { x: 550, y: 140 },
  },
};

let journeyIndex = 0;
setInterval(() => {
  journey(journeyIndex, journeyIndex);
  journeyIndex = (journeyIndex + 1) % 4;
}, 600);
