function addMessage(posX, posY, i) {
    let className = "message";
    if (i !== undefined) {
        className = className + '-' + i
    }
    const node = document.createElement('img');
    const MESSAGE_SIZE = 28;
    node.classList = className;
    node.src = ASSETS_PATH + 'message.svg';
    node.height = MESSAGE_SIZE;
    node.width = MESSAGE_SIZE;
    node.style.position = 'absolute';
    node.style.left = `-${MESSAGE_SIZE / 2}px`;
    node.style.top = `-${MESSAGE_SIZE / 2}px`;
    node.style.transform = `translateX(${posX}px) translateY(${posY}px)`
    frame.appendChild(node);
    return node;
}

function createLineConnection(fromX, fromY, toX, toY, withArrow, strokeWidth) {
    const line = document.createElementNS(
        "http://www.w3.org/2000/svg",
        "line"
    );
    line.setAttribute("x1", fromX);
    line.setAttribute("y1", fromY);
    line.setAttribute("x2", toX);
    line.setAttribute("y2", toY);
    line.style.stroke = "white";
    line.style.strokeWidth = strokeWidth;
    if (withArrow) {
        line.setAttribute("marker-end", "url(#triangle)");
    }
    return line;
}

function moveMessage(targets, timeline, delay, duration, targetPoints) {
    // move message by path
    targetPoints.forEach((nextPoint, index) => {
        timeline = timeline.add({
            targets: targets,
            translateX: nextPoint.x,
            translateY: nextPoint.y
        }, delay + duration*index);
    });
}

function getPoint(node) {
    if (typeof node === "string") {
        node = document.getElementById(node);
    }
    const svgRect = node.getBBox();
    return {
        id: node.id,
        query_id: '#' + node.id,
        node: node,
        x: svgRect.x + svgRect.width / 2,
        y: svgRect.y + svgRect.height / 2,
    };
}

function cloneNode(source, clone_id, startLocation) {
    const clone = source.node.cloneNode(true);
    clone.id = clone_id;
    clone.style.opacity = "0";
    if (startLocation) {
        clone.style.transform = `translateX(${startLocation.x - source.x}px) translateY(${startLocation.y - source.y}px)`;
    }
    source.node.parentNode.appendChild(clone);
    return getPoint(clone);
}