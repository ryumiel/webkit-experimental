App.Model = DS.Model;

App.NameLabelModel = DS.Model.extend({
    name: DS.attr('string'),
    label: function ()
    {
        return this.get('name');
    }.property('name'),
});

App.Test = App.NameLabelModel.extend({
    url: DS.attr('string'),
    parent: DS.belongsTo('test', {inverse: 'childTests'}),
    childTests: DS.hasMany('test', {inverse: 'parent'}),
    metrics: DS.hasMany('metrics'),
});

App.Metric = App.NameLabelModel.extend({
    test: DS.belongsTo('test'),
    aggregator: DS.attr('string'),
    label: function ()
    {
        return this.get('name') + (this.get('aggregator') ? ' : ' + this.get('aggregator') : '');
    }.property('name', 'aggregator'),
    path: function ()
    {
        var path = [];
        var parent = this.get('test');
        while (parent) {
            path.push(parent.get('name'));
            parent = parent.get('parent');
        }
        return path.reverse();
    }.property('name', 'test'),
    fullName: function ()
    {
        return this.get('path').join(' \u220b ') /* &ni; */
            + ' : ' + this.get('label');
    }.property('path', 'label'),
});

App.Builder = App.NameLabelModel.extend({
    buildUrl: DS.attr('string'),
    urlFromBuildNumber: function (buildNumber)
    {
        var template = this.get('buildUrl');
        return template ? template.replace(/\$builderName/g, this.get('name')).replace(/\$buildNumber/g, buildNumber) : null;
    }
});

App.BugTracker = App.NameLabelModel.extend({
    bugUrl: DS.attr('string'),
    newBugUrl: DS.attr('string'),
    repositories: DS.hasMany('repository'),
});

App.DateArrayTransform = DS.Transform.extend({
    deserialize: function (serialized)
    {
        return serialized.map(function (time) { return new Date(time); });
    }
});

App.Platform = App.NameLabelModel.extend({
    _metricSet: null,
    _testSet: null,
    metrics: DS.hasMany('metric'),
    lastModified: DS.attr('dateArray'),
    containsMetric: function (metric)
    {
        if (!this._metricSet)
            this._metricSet = new Ember.Set(this.get('metrics'));
        return this._metricSet.contains(metric);
    },
    lastModifiedTimeForMetric: function (metric)
    {
        var index = this.get('metrics').indexOf(metric);
        if (index < 0)
            return null;
        return this.get('lastModified').objectAt(index);
    },
    containsTest: function (test)
    {
        if (!this._testSet) {
            var set = new Ember.Set();
            this.get('metrics').forEach(function (metric) {
                for (var test = metric.get('test'); test; test = test.get('parent'))
                    set.push(test);
            });
            this._testSet = set;
        }
        return this._testSet.contains(test);
    },
});

App.Repository = App.NameLabelModel.extend({
    url: DS.attr('string'),
    blameUrl: DS.attr('string'),
    hasReportedCommits: DS.attr('boolean'),
    urlForRevision: function (currentRevision)
    {
        return (this.get('url') || '').replace(/\$1/g, currentRevision);
    },
    urlForRevisionRange: function (from, to)
    {
        return (this.get('blameUrl') || '').replace(/\$1/g, from).replace(/\$2/g, to);
    },
});

App.Dashboard = App.NameLabelModel.extend({
    serialized: DS.attr('string'),
    table: function ()
    {
        var json = this.get('serialized');
        try {
            var parsed = JSON.parse(json);
        } catch (error) {
            console.log("Failed to parse the grid:", error, json);
            return [];
        }
        if (!parsed)
            return [];
        return this._normalizeTable(parsed);
    }.property('serialized'),

    rows: function ()
    {
        return this.get('table').slice(1);
    }.property('table'),

    headerColumns: function ()
    {
        var table = this.get('table');
        if (!table || !table.length)
            return [];
        return table[0].map(function (name, index) {
            return {label:name, index: index};
        });
    }.property('table'),

    _normalizeTable: function (table)
    {
        var maxColumnCount = Math.max(table.map(function (column) { return column.length; }));
        for (var i = 1; i < table.length; i++) {
            var row = table[i];
            for (var j = 1; j < row.length; j++) {
                if (row[j] && !(row[j] instanceof Array)) {
                    console.log('Unrecognized (platform, metric) pair at column ' + i + ' row ' + j + ':' + row[j]);
                    row[j] = [];
                }
            }
        }
        return table;
    },
});

App.MetricSerializer = App.PlatformSerializer = DS.RESTSerializer.extend({
    normalizePayload: function (payload)
    {
        var results = {
            platforms: this._normalizeIdMap(payload['all']),
            builders: this._normalizeIdMap(payload['builders']),
            tests: this._normalizeIdMap(payload['tests']).map(function (test) {
                test['parent'] = test['parentId'];
                return test;
            }),
            metrics: this._normalizeIdMap(payload['metrics']),
            repositories: this._normalizeIdMap(payload['repositories']),
            bugTrackers: this._normalizeIdMap(payload['bugTrackers']),
            dashboards: [],
        };

        for (var testId in payload['tests']) {
            var test = payload['tests'][testId];
            var parent = payload['tests'][test['parent']];
            if (!parent)
                continue;
            if (!parent['childTests'])
                parent['childTests'] = [];
            parent['childTests'].push(testId);
        }

        for (var metricId in payload['metrics']) {
            var metric = payload['metrics'][metricId];
            var test = payload['tests'][metric['test']];
            if (!test['metrics'])
                test['metrics'] = [];
            test['metrics'].push(metricId);
        }

        var id = 1;
        var dashboardsInPayload = payload['dashboards'];
        for (var dashboardName in dashboardsInPayload) {
            results['dashboards'].push({
                id: id,
                name: dashboardName,
                serialized: JSON.stringify(dashboardsInPayload[dashboardName])
            });
            id++;
        }

        return results;
    },
    _normalizeIdMap: function (idMap)
    {
        var results = [];
        for (var id in idMap) {
            var definition = idMap[id];
            definition['id'] = id;
            results.push(definition);
        }
        return results;
    }
});

App.PlatformAdapter = DS.RESTAdapter.extend({
    buildURL: function (type, id)
    {
        return '../data/manifest.json';
    },
});

App.MetricAdapter = DS.RESTAdapter.extend({
    buildURL: function (type, id)
    {
        return '../data/manifest.json';
    },
});

App.Manifest = Ember.Controller.extend({
    platforms: null,
    topLevelTests: null,
    repositories: [],
    repositoriesWithReportedCommits: [],
    bugTrackers: [],
    _platformById: {},
    _metricById: {},
    _builderById: {},
    _repositoryById: {},
    _dashboardByName: {},
    _defaultDashboardName: null,
    _fetchPromise: null,
    fetch: function (store)
    {
        if (!this._fetchPromise)
            this._fetchPromise = store.findAll('platform').then(this._fetchedManifest.bind(this, store));
        return this._fetchPromise;
    },
    isFetched: function () { return !!this.get('platforms'); }.property('platforms'),
    platform: function (id) { return this._platformById[id]; },
    metric: function (id) { return this._metricById[id]; },
    builder: function (id) { return this._builderById[id]; },
    repository: function (id) { return this._repositoryById[id]; },
    dashboardByName: function (name) { return this._dashboardByName[name]; },
    defaultDashboardName: function () { return this._defaultDashboardName; },
    _fetchedManifest: function (store)
    {
        var startTime = Date.now();
        var self = this;

        var topLevelTests = [];
        store.all('test').forEach(function (test) {
            var parent = test.get('parent');
            if (!parent)
                topLevelTests.push(test);
        });
        this.set('topLevelTests', topLevelTests);

        store.all('metric').forEach(function (metric) {
            self._metricById[metric.get('id')] = metric;
        });
        var platforms = store.all('platform');
        platforms.forEach(function (platform) {
            self._platformById[platform.get('id')] = platform;
        });
        this.set('platforms', platforms);

        store.all('builder').forEach(function (builder) {
            self._builderById[builder.get('id')] = builder;
        });

        var repositories = store.all('repository');
        repositories.forEach(function (repository) {
            self._repositoryById[repository.get('id')] = repository;
        });
        this.set('repositories', repositories.sortBy('id'));
        this.set('repositoriesWithReportedCommits',
            repositories.filter(function (repository) { return repository.get('hasReportedCommits'); }));

        this.set('bugTrackers', store.all('bugTracker').sortBy('name'));

        var dashboards = store.all('dashboard').sortBy('name');
        this.set('dashboards', dashboards);
        dashboards.forEach(function (dashboard) { self._dashboardByName[dashboard.get('name')] = dashboard; });
        this._defaultDashboardName = dashboards.length ? dashboards[0].get('name') : null;
    },
    fetchRunsWithPlatformAndMetric: function (store, platformId, metricId, testGroupId, useCache)
    {
        Ember.assert("Can't cache results for test groups", !(testGroupId && useCache));
        var self = this;
        return Ember.RSVP.all([
            RunsData.fetchRuns(platformId, metricId, testGroupId, useCache),
            this.fetch(store),
        ]).then(function (values) {
            var response = values[0];

            var platform = App.Manifest.platform(platformId);
            var metric = App.Manifest.metric(metricId);

            return {
                platform: platform,
                metric: metric,
                data: response ? self._formatFetchedData(metric.get('name'), response.configurations) : null,
                shouldRefetch: !response || +response.lastModified < +platform.lastModifiedTimeForMetric(metric),
            };
        });
    },
    _formatFetchedData: function (metricName, configurations)
    {
        var suffix = metricName.match('([A-z][a-z]+|FrameRate)$')[0];
        var unit = {
            'FrameRate': 'fps',
            'Runs': '/s',
            'Time': 'ms',
            'Malloc': 'bytes',
            'Heap': 'bytes',
            'Allocations': 'bytes'
        }[suffix];

        var smallerIsBetter = unit != 'fps' && unit != '/s'; // Assume smaller is better for unit-less metrics.

        var useSI = unit == 'bytes';
        var unitSuffix = unit ? ' ' + unit : '';
        var deltaFormatterWithoutSign = useSI ? d3.format('.2s') : d3.format('.2g');

        return {
            current: configurations.current.timeSeriesByCommitTime(),
            baseline: configurations.baseline ? configurations.baseline.timeSeriesByCommitTime() : null,
            target: configurations.target ? configurations.target.timeSeriesByCommitTime() : null,
            unit: unit,
            formatWithUnit: function (value) { return this.formatter(value) + unitSuffix; },
            formatWithDeltaAndUnit: function (value, delta)
            {
                return this.formatter(value) + (delta && !isNaN(delta) ? ' \u00b1 ' + deltaFormatterWithoutSign(delta) : '') + unitSuffix;
            },
            formatter: useSI ? d3.format('.4s') : d3.format('.4g'),
            deltaFormatter: useSI ? d3.format('+.2s') : d3.format('+.2g'),
            smallerIsBetter: smallerIsBetter,
        };
    }
}).create();
