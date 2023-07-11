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

const icon_consumer_el = [
  document.querySelector("#icon-consumer-1 rect"),
  document.querySelector("#icon-consumer-2 rect"),
  document.querySelector("#icon-consumer-3 rect"),
  document.querySelector("#icon-consumer-4 rect"),
];

function journey(producerIndex, consumerIndex) {
  const msg = icon_message_el.cloneNode();
  msg.innerHTML = icon_message_el.innerHTML;
  msg.style.transformBox = "fill-box";
  msg.style.transformOrigin = "15px 15px";
  msg.style.display = "inherit";
  svg.appendChild(msg);

  const tl = anime.timeline({
    duration: 1000,
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
const priorities = [[0, 3], [2], [1]];
let outNodes = [];
setInterval(() => {
  journey(journeyIndex % 4, outNodes[journeyIndex % outNodes.length]);
  journeyIndex++;
}, 300);

const timeToDead = 2000;
const timelineEvents = () => {
  outNodes = priorities[0];

  setTimeout(() => {
    outNodes = [3];
    setTimeout(() => {
      icon_consumer_el[0].style.opacity = 0.2;
    }, timeToDead);
  }, 2000);

  setTimeout(() => {
    outNodes = [2];
    setTimeout(() => {
      icon_consumer_el[3].style.opacity = 0.2;
    }, timeToDead);
  }, 4000);

  setTimeout(() => {
    outNodes = [1];
    setTimeout(() => {
      icon_consumer_el[2].style.opacity = 0.2;
    }, timeToDead);
  }, 8000);

  setTimeout(() => {
    outNodes = priorities[0];
    setTimeout(() => {
      icon_consumer_el[0].style.opacity = 1;
      icon_consumer_el[2].style.opacity = 1;
      icon_consumer_el[3].style.opacity = 1;
    }, timeToDead);
  }, 11000);
};

timelineEvents();
setInterval(timelineEvents, 14000);
// }, 20000);
