const feed = document.getElementById("loot-feed");
const controls = document.getElementById("log-controls");
const receiptList = document.getElementById("receipt-list");
const feedStatus = document.getElementById("feed-status");
const receiptCount = document.getElementById("receipt-count");
const feedEmpty = document.getElementById("feed-empty");
const announcement = document.getElementById("feed-announcement");
let refreshTimer;
let pendingRequest;

function setStatus(status) {
    if (feedStatus.textContent !== status) {
        feedStatus.textContent = status;
        feedStatus.dataset.state = status.toLowerCase();
    }
}

function updateReceipts(incomingList) {
    const existing = new Map(Array.from(receiptList.children, row => [row.dataset.receipt, row]));
    let cursor = receiptList.firstElementChild;
    for (const incoming of Array.from(incomingList.children)) {
        const row = existing.get(incoming.dataset.receipt) ?? incoming;
        if (row !== cursor) {
            receiptList.insertBefore(row, cursor);
        }
        cursor = row.nextElementSibling;
    }
    while (cursor) {
        const obsolete = cursor;
        cursor = cursor.nextElementSibling;
        obsolete.remove();
    }
    feedEmpty.hidden = receiptList.children.length > 0;
}

async function refreshLoot() {
    const url = new URL(controls.action);
    const search = controls.elements.q.value.trim();
    const hostnameStatus = controls.elements.hostname_status.value;
    const sort = controls.elements.sort.value;
    if (search) url.searchParams.set("q", search);
    if (hostnameStatus) url.searchParams.set("hostname_status", hostnameStatus);
    if (sort === "oldest") url.searchParams.set("sort", sort);

    const request = new AbortController();
    pendingRequest = request;
    try {
        const response = await fetch(url, { cache: "no-store", signal: request.signal });
        if (!response.ok) throw new Error("Console refresh failed");

        const page = new DOMParser().parseFromString(await response.text(), "text/html");
        if (request.signal.aborted) return;
        const incomingFeed = page.getElementById("loot-feed");
        const incomingList = page.getElementById("receipt-list");
        const incomingOverview = page.getElementById("collection-overview");
        if (!incomingFeed || !incomingList || !incomingOverview) throw new Error("Loot log missing");

        const dataChanged = incomingFeed.dataset.revision !== feed.dataset.revision;
        const controlsChanged = url.search !== window.location.search;
        if (dataChanged) {
            document.getElementById("collection-overview").replaceWith(incomingOverview);
            feed.dataset.revision = incomingFeed.dataset.revision;
        }
        if (dataChanged || controlsChanged) {
            const before = Array.from(receiptList.children, row => row.dataset.receipt);
            updateReceipts(incomingList);
            receiptCount.textContent = page.getElementById("receipt-count").textContent;
            feedEmpty.textContent = page.getElementById("feed-empty").textContent;
            const after = Array.from(receiptList.children, row => row.dataset.receipt);
            if (before.length !== after.length || after.some((id, index) => id !== before[index])) {
                announcement.textContent = after.length ? "Loot Log updated." : "No captures to display.";
            }
        }
        if (controlsChanged) window.history.replaceState(window.history.state, "", url);
        setStatus("Live");
    } catch {
        if (!request.signal.aborted) setStatus("Retrying");
    } finally {
        if (!request.signal.aborted) {
            pendingRequest = null;
            scheduleRefresh(2000);
        }
    }
}

function scheduleRefresh(delay) {
    window.clearTimeout(refreshTimer);
    pendingRequest?.abort();
    refreshTimer = window.setTimeout(refreshLoot, delay);
}

controls.elements.q.addEventListener("input", () => scheduleRefresh(250));
controls.addEventListener("change", event => {
    if (event.target.matches("select")) scheduleRefresh(0);
});
controls.addEventListener("submit", event => {
    event.preventDefault();
    scheduleRefresh(0);
});
scheduleRefresh(2000);
