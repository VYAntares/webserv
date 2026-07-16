/* Mini moteur de tests pour la suite webserv.
 *
 * Usage dans une page :
 *   T.add("nom du test", "attendu", async () => {
 *       const r = await T.req("/files/hello.txt");
 *       T.assert(r.status === 200, "status " + r.status);
 *       return "status " + r.status;          // détail affiché si PASS
 *   });
 *   T.mount();                                 // rend la table dans #tests
 */
window.T = (function () {
	const tests = [];

	function add(name, expected, fn, opts) {
		tests.push({ name: name, expected: expected, fn: fn, opts: opts || {} });
	}

	function assert(cond, msg) {
		if (!cond) throw new Error(msg || "assertion échouée");
	}

	/* fetch enrobé : renvoie status + headers + texte, sans cache */
	async function req(url, opts) {
		const o = Object.assign({ cache: "no-store" }, opts || {});
		const res = await fetch(url, o);
		const text = await res.text();
		return {
			status: res.status,
			headers: res.headers,
			text: text,
			redirected: res.redirected,
			url: res.url,
		};
	}

	/* corps de taille exacte, sans '+' ni '%' (handlePost les décoderait) */
	function makeBody(size) {
		const chunk = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz-_".repeat(1024); // 64 KiB
		let out = "";
		while (out.length + chunk.length <= size) out += chunk;
		out += chunk.slice(0, size - out.length);
		return out;
	}

	function fmtBytes(n) {
		if (n >= 1048576) return (n / 1048576).toFixed(1) + " Mo";
		if (n >= 1024) return (n / 1024).toFixed(0) + " Ko";
		return n + " o";
	}

	/* ---------- rendu ---------- */
	let tbody = null, summaryEl = null;

	function setBadge(row, state, label) {
		const b = row.querySelector(".badge");
		b.className = "badge " + state;
		b.textContent = label;
	}

	async function runOne(i) {
		const t = tests[i];
		const row = tbody.children[i];
		setBadge(row, "run", "...");
		row.querySelector(".msg").textContent = "";
		try {
			const detail = await t.fn();
			t.state = "pass";
			setBadge(row, "pass", "PASS");
			row.querySelector(".msg").textContent = detail || "";
		} catch (e) {
			t.state = t.opts.warnOnly ? "warn" : "fail";
			setBadge(row, t.state, t.opts.warnOnly ? "WARN" : "FAIL");
			row.querySelector(".msg").textContent = e.message;
		}
		updateSummary();
	}

	function updateSummary() {
		const pass = tests.filter(t => t.state === "pass").length;
		const fail = tests.filter(t => t.state === "fail").length;
		const warn = tests.filter(t => t.state === "warn").length;
		summaryEl.innerHTML = "<span class='pass'>" + pass + " PASS</span> / " +
			"<span class='fail'>" + fail + " FAIL</span>" +
			(warn ? " / " + warn + " WARN" : "") +
			" — " + tests.length + " tests";
	}

	async function runAll(btn) {
		btn.disabled = true;
		for (let i = 0; i < tests.length; i++) {
			if (tests[i].opts.manual) continue;    // tests longs : bouton dédié
			await runOne(i);
		}
		btn.disabled = false;
	}

	function mount(sel) {
		const host = document.querySelector(sel || "#tests");
		const top = document.createElement("div");
		const runBtn = document.createElement("button");
		runBtn.className = "primary";
		runBtn.textContent = "▶ Tout lancer";
		runBtn.onclick = function () { runAll(runBtn); };
		summaryEl = document.createElement("span");
		summaryEl.className = "summary";
		summaryEl.style.marginLeft = "1rem";
		top.appendChild(runBtn);
		top.appendChild(summaryEl);
		host.appendChild(top);

		const table = document.createElement("table");
		table.className = "tests";
		table.innerHTML = "<thead><tr><th></th><th>Test</th><th>Attendu</th>" +
			"<th>Résultat</th><th>Détail</th></tr></thead>";
		tbody = document.createElement("tbody");
		tests.forEach(function (t, i) {
			const tr = document.createElement("tr");
			tr.innerHTML =
				"<td><button title='Lancer ce test'>▶</button></td>" +
				"<td class='name'></td><td class='detail'></td>" +
				"<td><span class='badge idle'>—</span></td>" +
				"<td class='detail msg'></td>";
			tr.querySelector(".name").textContent = t.name;
			tr.querySelectorAll(".detail")[0].textContent = t.expected;
			tr.querySelector("button").onclick = function () { runOne(i); };
			tbody.appendChild(tr);
		});
		table.appendChild(tbody);
		host.appendChild(table);
		updateSummary();
	}

	return { add: add, assert: assert, req: req, makeBody: makeBody,
	         fmtBytes: fmtBytes, mount: mount, tests: tests };
})();

/* Barre de navigation commune, injectée en haut de chaque page */
(function () {
	const pages = [
		["index.html",  "Accueil"],
		["get.html",    "GET & HEAD"],
		["post.html",   "POST"],
		["upload.html", "Upload"],
		["delete.html", "DELETE"],
		["cgi.html",    "CGI"],
		["errors.html", "Erreurs"],
		["stress.html", "Stress"],
		["bonus.html",  "Bonus"],
	];
	const here = location.pathname.split("/").pop() || "index.html";
	const nav = document.createElement("nav");
	let html = "<a class='brand' href='/default/'>webserv&nbsp;tests</a>";
	pages.forEach(function (p) {
		html += "<a href='/default/" + p[0] + "'" +
			(p[0] === here ? " class='current'" : "") + ">" + p[1] + "</a>";
	});
	nav.innerHTML = html;
	document.addEventListener("DOMContentLoaded", function () {
		document.body.insertBefore(nav, document.body.firstChild);
	});
})();
