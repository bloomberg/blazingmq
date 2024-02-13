import logging
import re
from typing import Any, Callable, List, Optional

from blazingmq.dev.configurator import Configurator

Tweak = Callable[[Configurator], None]

TWEAK_ATTRIBUTE = "__tweaks__"
SNAKE_RE = re.compile("([A-Z]+[a-z0-9]+)")

logger = logging.getLogger(__name__).parent


def decorator(tweak: Callable[[Configurator], None]):
    """
    Return a decorator that adds 'tweak' to 'target''s tweak list (creating it
    if necessary).
    """

    def apply(target: Any):
        attr = getattr(target, TWEAK_ATTRIBUTE, None)

        if attr is None:
            attr = []
            setattr(target, TWEAK_ATTRIBUTE, attr)

        attr.append((tweak, 0))

        return target

    return apply


class TweakMetaclass(type):
    """
    Metaclass to generate a '__call__' function adds a tweak to a function,
    object or class.
    """

    def __new__(cls, clsname, bases, dct):
        def call(self, value: Any):
            # "definition.parameters.max_queues"
            path = [
                SNAKE_RE.sub(r"_\1", name).lower()[1:]
                for name in type(self).__qualname__.split(".")
            ]
            _ = path.pop(0)
            leaf = path.pop()

            def tweak(configurator: Configurator) -> None:
                logger.debug("set %s to %s", path, value)

                obj = configurator.proto
                for attr in path:
                    obj = getattr(obj, attr)
                setattr(obj, leaf, value)

            return decorator(tweak)

        tweak_class = super(TweakMetaclass, cls).__new__(cls, clsname, bases, dct)
        setattr(tweak_class, "__call__", call)

        return tweak_class


from . import generated

tweak = generated.TweakFactory()
