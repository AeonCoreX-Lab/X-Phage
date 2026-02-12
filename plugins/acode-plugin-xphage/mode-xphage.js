define("ace/mode/xphage_highlight_rules", ["require", "exports", "module", "ace/lib/oop", "ace/mode/text_highlight_rules"], function(require, exports, module) {
    "use strict";
    var oop = require("../lib/oop");
    var TextHighlightRules = require("./text_highlight_rules").TextHighlightRules;

    var XPhageHighlightRules = function() {
        this.$rules = {
            "start": [
                { token: "keyword.control", regex: "\\b(pulse|core|scan|bypass|~link)\\b" },
                { token: "storage.type", regex: "\\b(atom|shadow|matrix)\\b" },
                { token: "support.function", regex: "\\b(beam)\\b" },
                { token: "string", regex: '"', next: "string" },
                { token: "comment", regex: "//.*$" },
                { token: "constant.numeric", regex: "\\b\\d+(\\.\\d+)?\\b" },
                { token: "keyword.operator", regex: "\\+|\\-|\\*|\\/|=" }
            ],
            "string": [
                { token: "string", regex: '"', next: "start" },
                { defaultToken: "string" }
            ]
        };
    };
    oop.inherits(XPhageHighlightRules, TextHighlightRules);
    exports.XPhageHighlightRules = XPhageHighlightRules;
});
