// Configuration
const ANIMATION_SPEED = 2000; // ms

function findPoints(query) {
    return [...document.querySelectorAll(query)].map(element => {
        return getPoint(element);
    }).sort((a, b) => (a.id > b.id) ? 1 : (b.id > a.id) ? -1 : 0);
}

const messageTemplate = getPoint('icon-message');
messageTemplate.node.style.opacity = "0.0";

function sendMessage(tl, step, from, toList) {
    const fromPoint = getPoint(from);
    toList.forEach(el => {
        const message = cloneNode(messageTemplate, `msg-${from}-${el}`, fromPoint);
        const targetPoint = getPoint(document.getElementById(el));

        tl.add({
            targets: message.query_id,
            translateX: targetPoint.x - message.x,
            translateY: targetPoint.y - message.y,
            opacity: [
                { value: 1, duration: 1 },
                { value: 1, duration: ANIMATION_SPEED - 1 },
                { value: 0, duration: 1 },
            ]
        }, ANIMATION_SPEED * step);
    });
}

const tl = anime.timeline({
    duration: ANIMATION_SPEED,
    loop: true,
    easing: "linear",
});

sendMessage(tl, 0, 'icon-primary', ['icon-replica-1', 'icon-replica-2', 'icon-replica-3']);

sendMessage(tl, 1, 'icon-replica-1', ['icon-proxy-1', 'icon-proxy-2']);
sendMessage(tl, 1, 'icon-replica-2', ['icon-proxy-3', 'icon-proxy-4']);
sendMessage(tl, 1, 'icon-replica-3', ['icon-proxy-5', 'icon-proxy-6']);

sendMessage(tl, 2, 'icon-proxy-1', ['icon-consumer-1', 'icon-consumer-2']);
sendMessage(tl, 2, 'icon-proxy-2', ['icon-consumer-3', 'icon-consumer-4']);
sendMessage(tl, 2, 'icon-proxy-3', ['icon-consumer-5', 'icon-consumer-6', 'icon-consumer-7']);
sendMessage(tl, 2, 'icon-proxy-4', ['icon-consumer-8', 'icon-consumer-9', 'icon-consumer-10']);
sendMessage(tl, 2, 'icon-proxy-5', ['icon-consumer-11', 'icon-consumer-12']);
sendMessage(tl, 2, 'icon-proxy-6', ['icon-consumer-13', 'icon-consumer-14', 'icon-consumer-15', 'icon-consumer-16']);
