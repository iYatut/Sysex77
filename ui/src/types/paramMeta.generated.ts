export interface ParamMeta {
    id:         string;
    tag:        string;
    uiLabel:    string;
    group:      string;
    perElement: boolean;
    rawMin:     number;
    rawMax:     number;
    decode:     string;
    encode:     string;
    enumNames:  string[];
    addr:       Addr;
    confidence: Confidence;
}

export interface Addr {
    b3:         number;
    b4:         number;
    b5:         number;
    b6:         number;
    perElement: boolean;
}

export interface Confidence {
    confirmedLive:     boolean;
    confirmedBulk8101: boolean;
    confirmedBulk0040: boolean;
    candidateBulk0040: boolean;
    packedConflict:    boolean;
}
