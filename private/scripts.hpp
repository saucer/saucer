#pragma once

#include <string_view>

namespace saucer::scripts
{
    static constexpr std::string_view webview_script = R"js(
    window.saucer = 
    {{
        windowEdge:
        {{
            top:    1 << 0,
            bottom: 1 << 1,
            left:   1 << 2,
            right:  1 << 3,
        }},
        internal: 
        {{
            {internal}
        }},
        startDrag: async () =>
        {{
            await window.saucer.internal.send_message(JSON.stringify({{
                ["saucer:drag"]: true
            }}));
        }},
        startResize: async (edge) =>
        {{
            await window.saucer.internal.send_message(JSON.stringify({{
                ["saucer:resize"]: true,
                edge,
            }}));
        }}
    }};

    document.addEventListener("mousedown", async ({{ x, y, target, button }}) => {{
        if (button !== 0)
        {{
            return;
        }}

        if (target.hasAttribute("data-webview-ignore"))
        {{
            return;
        }}

        const dragger  = [...document.querySelectorAll("[data-webview-drag]")];
        const resizer  = [...document.querySelectorAll("[data-webview-resize]")];
        const elements = document.elementsFromPoint(x, y);

        if (elements.some(x => dragger.includes(x)))
        {{
            await window.saucer.startDrag();
            return;
        }}

        const resize = elements.find(x => resizer.includes(x));

        if (!resize)
        {{
            return;
        }}

        const attributes = [...resize.attributes];
        const attribute  = attributes.find(x => x.name === "data-webview-resize");

        const edges      = Object.keys(window.saucer.windowEdge);
        const edge       = [...attribute.value].reduce((flag, value) => flag | window.saucer.windowEdge[edges.find(x => x.startsWith(value))], 0);

        await window.saucer.startResize(edge);
    }});
    )js";

    static constexpr std::string_view smartview_script = R"js(
    window.saucer.internal.idc = 0;
    window.saucer.internal.rpc = [];

    window.saucer.internal.resolve = async (id, value) =>
    {{
        await window.saucer.internal.send_message({serializer}({{
                ["saucer:resolve"]: true,
                id,
                result: value === undefined ? null : value,
        }}));
    }}
    
    window.saucer.call = async (name, params) =>
    {{
        if (!Array.isArray(params))
        {{
            throw 'Bad arguments, expected array';
        }}

        if (typeof name !== 'string' && !(name instanceof String))
        {{
            throw 'Bad name, expected string';
        }}

        const id = ++window.saucer.internal.idc;
        
        const rtn = new Promise((resolve, reject) => {{
            window.saucer.internal.rpc[id] = {{
                reject,
                resolve,
            }};
        }});

        await window.saucer.internal.send_message({serializer}({{
                ["saucer:call"]: true,
                id,
                name,
                params,
        }}));

        return rtn;
    }}

    window.saucer.exposed = new Proxy({{}}, {{
        get: (_, prop) => (...args) => window.saucer.call(prop, args),
    }});
    )js";
} // namespace saucer::scripts
